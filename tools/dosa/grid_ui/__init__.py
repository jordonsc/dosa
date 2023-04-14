from kivy.app import App
from kivy.uix.widget import Widget
from kivy.clock import Clock
from dosa.power import thresholds
import json


class MainWidget(Widget):
    data_file = "/usr/share/power_grid.json"
    pv_size = 1000
    bat_size = 520

    col_bg = (0.05, 0.04, 0.03, 1.0)
    col_text = (1.0, 1.0, 1.0, 1.0)
    col_good = (0.086, 0.584, 0.275, 1.0)
    col_warn = (0.580, 0.329, 0.137, 1.0)
    col_bad = (0.584, 0.102, 0.114, 1.0)
    col_special = (0.259, 0.384, 0.678, 1.0)
    col_supply = (0.580, 0.533, 0.196, 1.0)

    bat_soc = 0
    bat_ah_remaining = 0
    bat_voltage = 0

    mains_active: False

    pv_power = 0
    pv_voltage = 0

    load_power = 0
    load_current = 0

    def remove_splash(self, dt):
        self.remove_widget(self.ids.splash)

    def set_pv_size(self, watts):
        print("Update PV grid size to {} w".format(round(watts)))
        self.pv_size = watts
        self.update_pv(self.pv_power, self.pv_voltage)

    def set_battery_size(self, ah):
        print("Update battery array size to {} ah".format(round(ah)))
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

    def update_time(self, time):
        time_remaining = self.ids.time_remaining
        clock = self.ids.clock

        if time == 0:
            time_remaining.text = "-"
            time_remaining.color = self.col_text
            clock.source = "../assets/clock.png"
        else:
            abs_time = abs(time)
            unit = " hrs"
            if abs_time > 24:
                abs_time = abs_time/24
                unit = " days"

            time_remaining.text = str(round(abs_time, 1)) + unit
            if time > 0:
                time_remaining.color = self.col_good
                clock.source = "../assets/bat_full.png"
            else:
                clock.source = "../assets/bat_empty.png"
                if time < -10:
                    time_remaining.color = self.col_text
                elif time < -5:
                    time_remaining.color = self.col_warn
                else:
                    time_remaining.color = self.col_bad

    def reload_data(self, dt):
        try:
            with open(self.data_file, 'r', encoding='utf-8') as f:
                grid_data = json.load(f)
        except FileNotFoundError:
            self.set_error_condition("grid data missing")
            return
        except json.decoder.JSONDecodeError:
            self.set_error_condition("grid data file corrupted")
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
            if self.load_power < 0:
                self.update_time(self.bat_ah_remaining / (self.load_power / 12.5))
            elif self.load_power == 0:
                self.update_time(0)
            else:
                self.update_time((self.bat_size - self.bat_ah_remaining) / (self.load_power / 12.5))

            self.ids.splash.opacity = 0
        except KeyError:
            self.set_error_condition("Key error in grid data")

    def set_error_condition(self, err_msg="unknown"):
        self.ids.splash.opacity = 1
        print("ERROR: {}".format(err_msg))


class GridApp(App):
    def build(self):
        app = MainWidget()
        Clock.schedule_interval(app.reload_data, 5.0)

        return app
