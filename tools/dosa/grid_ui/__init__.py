from kivy.app import App
from kivy.uix.widget import Widget
from kivy.clock import Clock
from kivy.graphics import Line

import dosa
from dosa.power import thresholds, is_production_machine, get_data_file, get_config_file

from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler

import json
import logging
import os
import queue
import time

# Time in seconds to display the splash screen
# If there is no data ready yet, there may be additional delay
splash_time = 5.0 if is_production_machine() else 0.5

colors = {
    'default_background': (0.05, 0.04, 0.03, 1.0),
    'splash_green': (0.14, 0.23, 0.18, 1.0),
    'splash_green_high': (0.4, 0.74, 0.27, 1.0),
    'splash_green_low': (0.09, 0.16, 0.13, 1.0),
    'white': (1.0, 1.0, 1.0, 1.0),
    'green': (0.086, 0.584, 0.275, 1.0),
    'amber': (0.580, 0.329, 0.137, 1.0),
    'dark_amber': (0.580, 0.533, 0.196, 1.0),
    'red': (0.584, 0.102, 0.114, 1.0),
    'blue': (0.259, 0.384, 0.678, 1.0),
    'red_trans': (0.584, 0.102, 0.114, 0.5),
}


# Watchdog handler for the data file
class WatchHandler(FileSystemEventHandler):
    def __init__(self, q: queue.Queue) -> None:
        self.queue = q
        super().__init__()

    def on_modified(self, event):
        self.queue.put(event.src_path)


# Main screen widget
class MainWidget(Widget):
    cfg = {}

    pv_size = 1000
    bat_size = 520

    col_bg = colors['default_background']
    col_text = colors['white']
    col_good = colors['green']
    col_warn = colors['amber']
    col_bad = colors['red']
    col_special = colors['blue']
    col_supply = colors['dark_amber']

    bat_soc = 0
    bat_ah_remaining = 0
    bat_voltage = 0

    mains_active: False

    pv_power = 0
    pv_voltage = 0

    load_power = 0
    load_current = 0

    file_observer = None
    file_handler = None
    queue = queue.Queue()

    brightness_timeout = 0
    brightness_last_action = 0
    display_dimmed = False

    def begin_main_sequence(self, _):
        """
        To be called when you want to start the main display; begin polling & remove the splash screen.
        """
        if self.file_handler is not None:
            return

        self.file_handler = WatchHandler(self.queue)
        self.brightness_last_action = int(time.time())

        # We need the file to exist in order to monitor it for change
        if not os.path.exists(get_data_file()) or not os.path.isfile(get_data_file()):
            logging.error("Data file not found, creating empty data file")
            with open(get_data_file(), 'w', encoding='utf-8') as f:
                json.dump("{}", f, ensure_ascii=False)

        # Will load whatever is in the file and clear the splash
        self.reload_data()

        # Start a file watch
        self.file_observer = Observer()
        self.file_observer.schedule(self.file_handler, path=get_data_file(), recursive=False)
        self.file_observer.start()

        # Run the app main loop
        Clock.schedule_interval(self.main_loop, 0)

    def main_loop(self, _):
        """
        Main loop.

        Monitors things like files changed, or dims the display after time.
        """
        self.check_watchdog()

    def check_display(self):
        """
        If enough time has passed, dim the display
        """
        if self.brightness_timeout != 0 and (int(time.time()) - self.brightness_last_action > self.brightness_timeout):
            self.change_display_brightness(True)

    def check_watchdog(self):
        """
        Checks for data on the Watchdog queue.

        If data is found, this indicates that the data file has been updated by the main grid daemon.
        """
        try:
            # Will throw an exception if not found, else we just want to log the filename at best
            if self.queue.get_nowait() == get_data_file():
                logging.debug("Data file updated, reloading")
            self.reload_data()

        except queue.Empty:
            return

    def set_pv_size(self, watts):
        logging.info("Update PV grid size to {} w".format(round(watts)))
        self.pv_size = watts
        self.update_pv(self.pv_power, self.pv_voltage)

    def set_battery_size(self, ah):
        logging.info("Update battery array size to {} ah".format(round(ah)))
        self.bat_size = ah
        self.update_battery_soc(self.bat_soc, self.bat_voltage)

    def update_battery_soc(self, soc, volts):
        self.bat_soc = soc
        self.bat_ah_remaining = self.bat_size * self.bat_soc / 100
        self.bat_voltage = volts
        self.update_battery_stats()

    def update_battery_ah(self, amps, volts):
        self.bat_soc = int(round(amps / self.bat_size * 100))
        self.bat_ah_remaining = amps
        self.bat_voltage = volts
        self.update_battery_stats()

    def update_battery_stats(self):
        bs = self.ids.bat_soc
        bs.text = str(round(self.bat_soc)) + "%"
        if self.bat_soc == thresholds["battery"]["high"]:
            bs.color = self.col_special
        elif self.bat_soc >= thresholds["battery"]["med"]:
            bs.color = self.col_good
        elif self.bat_soc >= thresholds["battery"]["low"]:
            bs.color = self.col_warn
        else:
            bs.color = self.col_bad

        self.ids.bat_ah_remaining.text = str(int(round(self.bat_ah_remaining))) + "ah"
        self.ids.bat_voltage.text = str(round(self.bat_voltage, 1)) + "v"
        self.set_soc_graphic(round(self.bat_soc))

    def update_mains(self, active):
        self.mains_active = active

        mains = self.ids.mains_status

        if self.mains_active is None:
            mains.text = "Pending"
            mains.color = self.col_text
        elif self.mains_active:
            mains.text = "ACTIVE"
            mains.color = self.col_bad
        else:
            mains.text = "Inactive"
            mains.color = self.col_special

    def update_pv(self, watts, volts):
        self.pv_power = watts
        self.pv_voltage = volts

        self.ids.pv_power.text = str(round(self.pv_power)) + "w"
        self.ids.pv_voltage.text = str(round(self.pv_voltage, 1)) + "v"

        if self.pv_power >= self.pv_size * thresholds["pv"]["high"]:
            self.ids.pv_power.color = self.col_special
        elif self.pv_power >= self.pv_size * thresholds["pv"]["med"]:
            self.ids.pv_power.color = self.col_good
        elif self.pv_power >= self.pv_size * thresholds["pv"]["low"]:
            self.ids.pv_power.color = self.col_warn
        else:
            self.ids.pv_power.color = self.col_bad

    def update_load(self, watts, amps):
        self.load_power = watts
        self.load_current = amps

        ls = self.ids.load_status
        if self.load_power > 10:
            ls.text = "Charging"
            ls.color = self.col_good
        elif self.load_power < -10:
            ls.text = "Draining"
            ls.color = self.col_bad
        else:
            ls.text = "Steady"
            ls.color = self.col_text

        self.ids.load_power.text = str(int(round(self.load_power))) + "w"
        self.ids.load_current.text = str(round(self.load_current, 1)) + "a"

    def update_time(self, t):
        time_remaining = self.ids.time_remaining
        clock = self.ids.clock

        if t == 0:
            time_remaining.text = "-"
            time_remaining.color = self.col_text
            clock.source = "../assets/clock.png"
        else:
            abs_time = abs(t)
            unit = " mins"
            if abs_time > 60:
                abs_time = abs_time / 60
                unit = " hrs"
            if abs_time > 24:
                abs_time = abs_time / 24
                unit = " days"

            time_remaining.text = str(round(abs_time, 1)) + unit
            if t > 0:
                time_remaining.color = self.col_good
                clock.source = "../assets/bat_full.png"
            else:
                clock.source = "../assets/bat_empty.png"
                if t < -600:
                    time_remaining.color = self.col_text
                elif t < -300:
                    time_remaining.color = self.col_warn
                else:
                    time_remaining.color = self.col_bad

    def reload_data(self):
        """
        Loads the data file and updates the display with the information contained.
        """
        try:
            with open(get_data_file(), 'r', encoding='utf-8') as f:
                grid_data = json.load(f)
        except FileNotFoundError:
            self.set_error_condition("Grid data missing")
            return
        except json.decoder.JSONDecodeError:
            self.set_error_condition("Grid data file corrupted")
            return

        try:
            # Battery metrics
            if self.bat_size != grid_data["battery"]["capacity"]:
                self.set_battery_size(grid_data["battery"]["capacity"])
            # Change this to whatever is the most reliable metric
            self.update_battery_soc(grid_data["battery"]["soc"], grid_data["battery"]["voltage"])

            # Mains
            self.update_mains(grid_data["mains"]["active"])

            # PV metrics
            if self.pv_size != grid_data["pv"]["capacity"]:
                self.set_pv_size(grid_data["pv"]["capacity"])
            self.update_pv(grid_data["pv"]["power"], grid_data["pv"]["voltage"])

            # Load metrics
            self.update_load(grid_data["load"]["power"], grid_data["load"]["current"])

            # Estimate time remaining
            self.update_time(grid_data["load"]["ttg"])

            self.ids.splash.opacity = 0
        except (KeyError, TypeError) as e:
            self.set_error_condition(f"Key error in grid data: {e}")

    def set_soc_graphic(self, soc):
        soc_graphic = self.ids.soc_graphic
        soc_graphic.canvas.remove_group("soc_lines")
        soc_graphic.canvas.add(Line(circle=(soc_graphic.center_x, soc_graphic.center_y, 150 / 2, 240,
                                            dosa.map_range(soc, 0, 50, 240, 360)), group="soc_lines", width=4))
        if soc >= 50:
            soc_graphic.canvas.add(Line(circle=(soc_graphic.center_x, soc_graphic.center_y, 150 / 2, 0,
                                                dosa.map_range(soc, 50, 100, 0, 120)), group="soc_lines",
                                        width=4))

    def set_error_condition(self, err_msg="unknown"):
        self.ids.splash.opacity = 1
        logging.error(err_msg)

    def load_config(self):
        def get_value(arr, key, default):
            if key in arr:
                return arr[key]
            else:
                return default

        def get_value_range(arr, key, default: int, low: int, high: int):
            return min(max(low, int(get_value(arr, key, default))), high)

        logging.debug("Loading config")
        try:
            with open(get_config_file(), 'r', encoding='utf-8') as f:
                cfg = json.load(f)
                self.cfg["mains"] = get_value_range(cfg, "mains", 0, 0, 2)
                self.cfg["mains_opt"] = get_value_range(cfg, "mains_opt", 1, 0, 2)
                self.cfg["display"] = get_value_range(cfg, "display", 0, 0, 3)

                self.cfg["opt_0_title"] = get_value(cfg, "opt_0_title", "Eco")
                self.cfg["opt_1_title"] = get_value(cfg, "opt_1_title", "Std")
                self.cfg["opt_2_title"] = get_value(cfg, "opt_2_title", "Safe")

                self.ids.settings.ids.mains_0.text = self.cfg["opt_0_title"]
                self.ids.settings.ids.mains_1.text = self.cfg["opt_1_title"]
                self.ids.settings.ids.mains_2.text = self.cfg["opt_2_title"]

                self.ids.settings.ids[f"ctrl_{self.cfg['mains']}"].state = "down"
                self.ids.settings.ids[f"mains_{self.cfg['mains_opt']}"].state = "down"
                self.ids.settings.ids[f"display_{self.cfg['display']}"].state = "down"

        except FileNotFoundError:
            logging.warning(f"No config file found ({get_config_file()})")
            self.load_default_config()
            return
        except json.decoder.JSONDecodeError:
            logging.error(f"Error parsing config file ({get_config_file()})")
            self.load_default_config()
            return

    def load_default_config(self):
        self.cfg["mains"] = 0
        self.cfg["mains_opt"] = 1
        self.cfg["display"] = 0
        self.cfg["opt_0_title"] = "Eco"
        self.cfg["opt_1_title"] = "Std"
        self.cfg["opt_2_title"] = "Safe"
        self.ids.settings.ids["ctrl_{}".format(self.cfg["mains"])].state = "down"
        self.ids.settings.ids["mains_{}".format(self.cfg["mains_opt"])].state = "down"
        self.save_config()

    def save_config(self):
        logging.info(f"Writing config to {get_config_file()}")
        with open(get_config_file(), 'w', encoding='utf-8') as f:
            json.dump(self.cfg, f, ensure_ascii=False, indent=4)

    def ctrl_button(self, button: int):
        title = self.ids.settings.ids[f"ctrl_{button}"].text
        logging.info(f"Change mains control setting to option {button + 1} ({title})")
        self.ids.settings.ids[f"ctrl_{button}"].state = "down"
        self.cfg["mains"] = button
        self.save_config()

    def mains_button(self, button: int):
        title = self.cfg[f"opt_{button}_title"]
        logging.info(f"Change mains sensitivity setting to option {button + 1} ({title})")
        self.ids.settings.ids[f"mains_{button}"].state = "down"
        self.cfg["mains_opt"] = button
        self.save_config()

    def display_button(self, button: int):
        title = self.ids.settings.ids[f"display_{button}"].text
        logging.info(f"Change display setting to option {button + 1} ({title})")
        self.ids.settings.ids[f"display_{button}"].state = "down"
        self.cfg["display"] = button
        self.save_config()

    def display_touch(self, _, touch):
        self.brightness_last_action = int(time.time())
        if self.display_dimmed:
            self.change_display_brightness(False)

        if self.ids.settings.opacity != 0:
            return

        if touch.is_double_tap:
            logging.debug("Show settings screen")
            self.ids.settings.opacity = 1
            # Move the `disabled` change to the next frame to prevent a setting from being changed by the double-tap
            Clock.schedule_once(self.enable_settings, 0)

    def enable_settings(self, _):
        """
        Removes the disabled status to allow button presses.

        This prevents pressing settings buttons by mistake when double-tapping to bring up the settings widget.
        """
        self.ids.settings.disabled = False

    def change_display_brightness(self, dim: bool):
        """
        Change the display brightness.

        TODO: Not implemented.
        """
        pass


# Settings widget
class SettingsWidget(Widget):
    col_bg = colors['splash_green']
    col_text = colors['white']
    col_special = colors['blue']

    def home_button_press(self, _, touch):
        if self.ids.home_img.collide_point(*touch.pos):
            logging.debug("Home pressed")
            self.opacity = 0
            self.disabled = True


class GridApp(App):
    def build(self):
        self.title = "Power Grid"
        app = MainWidget()
        app.load_config()
        Clock.schedule_once(app.begin_main_sequence, splash_time)
        return app
