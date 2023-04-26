import json
import logging
import os
import queue
import struct
import time
from threading import Thread

import serial
from watchdog.events import FileSystemEventHandler
from watchdog.observers import Observer

import dosa
from dosa import LogLevel
from dosa.grid.bt1 import Bt1Client
from dosa.grid.exceptions import *
from dosa.power import thresholds, get_data_file, get_config_file


# Watchdog handler for the config file
class WatchHandler(FileSystemEventHandler):
    def __init__(self, q: queue.Queue) -> None:
        self.queue = q
        super().__init__()

    def on_modified(self, event):
        self.queue.put(event.src_path)


class StickConfig:
    stick_led_count = 8
    total_led_count = 24
    fps = 15

    colour_load = (0, 100, 100)
    colour_special = (0, 30, 170)
    colour_good = (0, 150, 0)
    colour_warn = (120, 60, 0)
    colour_bad = (160, 0, 0)
    colour_none = (0, 0, 0)

    index_mains = None  # None implies we're not using this LED bank
    index_bat = 16
    index_pv = 8
    index_load = 0

    threshold_load_warn = 180
    threshold_load_bad = 240

    pwm_min = 20
    pwm_max = 100
    low_temp = 25
    high_temp = 45
    warn_temp = 55
    error_temp = 60


class GridStatus:
    def __init__(self, data: dict = None):
        # Battery metrics
        self.battery_soc = 0  # 0-100 (uint)
        self.battery_voltage = 0

        # PV metrics
        self.pv_power = 0
        self.pv_voltage = 0

        # Load metrics
        self.load_power = 0
        self.load_current = 0
        self.time_remaining = 0  # in minutes

        # Controller metrics
        self.load_state = False
        self.controller_temperature = 0

        if data is not None:
            self.from_data(data)

    def from_data(self, data: dict,
                  allow_battery_data: bool = True,
                  allow_pv_data: bool = True,
                  allow_load_data: bool = True,
                  allow_controller_data: bool = True) -> int:
        """
        Updates metrics from a dict object.

        You can optionally select to ignore certain metric groups if you are gathering data from multiple sources.

        Returns the number of data-points modified.
        """
        changed = []

        def assign(original, key):
            if key in data:
                if data[key] != original:
                    changed.append(key)
                return data[key]

            return original

        if allow_battery_data:
            self.battery_soc = assign(self.battery_soc, 'battery_soc')
            self.battery_voltage = assign(self.battery_voltage, 'battery_voltage')

        if allow_pv_data:
            self.pv_power = assign(self.pv_power, 'pv_power')
            self.pv_voltage = assign(self.pv_voltage, 'pv_voltage')

        if allow_load_data:
            self.load_power = assign(self.load_power, 'load_power')
            self.load_current = assign(self.load_current, 'load_current')
            self.time_remaining = assign(self.time_remaining, 'time_remaining')

        if allow_controller_data:
            self.load_state = assign(self.load_state, 'load_state')
            self.controller_temperature = assign(self.controller_temperature, 'controller_temperature')

        return len(changed)

    def to_bytes(self) -> bytes:
        payload = b''
        payload += struct.pack("<B", round(self.battery_soc))
        payload += struct.pack("<H", round(self.battery_voltage * 10))

        payload += struct.pack("<H", self.pv_power)
        payload += struct.pack("<H", round(self.pv_voltage * 10))

        payload += struct.pack("<h", self.load_power)
        payload += struct.pack("<h", round(self.load_current * 10))
        payload += struct.pack("<h", self.time_remaining)

        payload += struct.pack("<B", int(self.load_state))
        payload += struct.pack("<h", self.controller_temperature)

        return payload


class PowerGrid:
    def __init__(self, tgt_mac, hci="hci0", poll_int=30, comms=None, stick=None, grid_size=1000, bat_size=500,
                 pwm_port=None, shunt_port=None):
        logging.basicConfig(level=logging.DEBUG)

        if comms is None:
            comms = dosa.Comms()
        self.comms = comms

        self.pv_size = grid_size
        self.bat_size = bat_size

        """
        Define the way we manage mains backup power:
          0: Automatic
          1: Always enabled
          2: Always disable
        """
        self.mains_setting = 0

        """
        Set the sensitivity for the automatic mode on the above setting:
          0: Eco mode; allow the SOC to get a reasonably low level, maximising solar usage
          1: Standard; a middle-ground, enable mains before we lose too much power
          2: Safe mode; enable the mains at a high-level, for when you want to use the grid as an emergency backup
        """
        self.mains_config_level = 0

        self.mains_active = None
        self.mains_proposed_state = None
        self.mains_proposal_time = None

        self.power_grid = GridStatus()

        # BLE
        self.hci = hci
        self.target_mac = tgt_mac
        self.poll_interval = poll_int  # BLE read data interval (seconds)
        self.bt_thread = None
        self.time_ble_daemon_died = None

        # LED stick
        self.stick = stick
        self.config = StickConfig()

        # Fan serial comms
        self.fan_speed = 0
        self.pwm_port = pwm_port
        self.pwm_serial = None
        self.fan_warning_time = None

        # Victron SmartShunt serial comms
        self.shunt_port = shunt_port
        self.shunt_serial = None
        self.shunt_last_connect_attempt = None
        self.shunt_last_updated = None
        self.shunt_new_metrics = 0

        # Threshold config
        self.config.threshold_pv_special = thresholds["pv"]["high"] * grid_size
        self.config.threshold_pv_good = thresholds["pv"]["med"] * grid_size
        self.config.threshold_pv_med = thresholds["pv"]["low"] * grid_size

        # Config file monitoring
        self.config_queue = queue.Queue()
        self.watch_handler = WatchHandler(self.config_queue)
        self.watch_observer = Observer()

        # Start brining systems online
        self.init_lights()
        self.init_config_watch()
        self.load_config()
        self.set_mains(None)

        if self.pwm_port:
            self.init_pwm_serial()
        else:
            logging.warning("No PWM serial port configured")

        if self.shunt_port:
            self.init_shunt_serial()
        else:
            logging.warning("No SmartShunt serial port configured")

    def init_lights(self):
        if self.stick is None:
            logging.warning("No lights configured")
            return

        logging.info("Bringing lights online..")
        self.stick.pixelsFill(self.stick.rgbColour(*self.config.colour_load))
        self.stick.pixelsShow()

    def init_pwm_serial(self):
        """
        Connect to the fan PWM micro-controller.

        Immediately return if we're already connected.

        NB: there is no delay-retry logic here, if you hammer this function, it will hammer a connection attempt.
        """
        if not self.pwm_port or self.pwm_serial is not None:
            return

        try:
            logging.info("Bringing PWM serial online..")
            self.pwm_serial = serial.Serial(self.pwm_port, 9600, timeout=1)
            self.pwm_serial.reset_input_buffer()
        except serial.serialutil.SerialException:
            logging.error("Failed to bring PWM serial online")

    def init_shunt_serial(self):
        """
        Connect to the Victron SmartShunt device.

        Immediately returns if we're already connected. If we recently tried to connect, we will wait a few seconds.
        """
        if not self.shunt_port or self.shunt_serial is not None:
            return

        # Delay-retry
        if (self.shunt_last_connect_attempt is not None) and (time.time() - self.shunt_last_connect_attempt < 5):
            return

        self.shunt_last_connect_attempt = time.time()

        try:
            logging.info("Bringing SmartShunt serial online..")
            self.shunt_serial = serial.Serial(self.shunt_port, 19200, parity=serial.PARITY_NONE,
                                              stopbits=serial.STOPBITS_ONE, bytesize=serial.EIGHTBITS, timeout=1)
            self.shunt_serial.reset_input_buffer()
        except serial.serialutil.SerialException:
            logging.error("Failed to bring SmartShunt serial online")

    def init_config_watch(self):
        # We need the file to exist in order to monitor it for change
        if not os.path.exists(get_config_file()) or not os.path.isfile(get_config_file()):
            logging.error("Config file not found, creating empty data file")
            with open(get_config_file(), 'w', encoding='utf-8') as f:
                json.dump("{}", f, ensure_ascii=False)

        self.watch_observer.schedule(self.watch_handler, path=get_config_file(), recursive=False)
        self.watch_observer.start()

    def run(self):
        """
        Primary loop.

        Runs indefinitely managing all sub-systems.
        """
        logging.info("Starting DOSA Power Grid Controller..")
        self.spawn_bt_listener()
        self.comms.net_log(LogLevel.INFO, "Power Grid online")

        def run_safe(fn, *args):
            try:
                fn(*args)
            except Exception as e:
                # Main thread should not error - if it does, understand why -
                logging.error(e)
                self.comms.net_log(LogLevel.ERROR, f"Main loop error: {e}")

        while True:
            run_safe(self.process_ble)
            run_safe(self.process_pwm_serial)
            run_safe(self.process_shunt_serial)
            run_safe(self.process_config)
            run_safe(self.process_udp)

    def process_ble(self):
        """
        Ensure the BLE manager thread is alive and healthy.

        Will respawn it after a cooling period if it crashes for any reason.
        """
        if not self.bt_thread.is_alive():
            if self.time_ble_daemon_died is not None:
                if time.time() - self.time_ble_daemon_died > 5:
                    logging.info("Respawning BT listener..")
                    self.time_ble_daemon_died = None
                    self.spawn_bt_listener()
            else:
                logging.warning("BT listener died, respawning in 5 seconds..")
                self.comms.net_log(LogLevel.WARNING, "BT listener died")
                self.time_ble_daemon_died = time.time()

    def process_pwm_serial(self):
        """
        Process data on PWM serial line.

        This is simply just logging a response from the PWM MCU, and potentially killing the connection if there was
        an error.

        If this serial connection has disconnected (or never connected), we will attempt to reconnect it when we want
        to update the fan speed next.
        """
        if self.pwm_serial is not None:
            try:
                while self.pwm_serial.in_waiting > 0:
                    try:
                        logging.debug("<FAN_CTRL: {}>".format(self.pwm_serial.readline().decode('utf-8').rstrip()))
                    except UnicodeDecodeError as e:
                        logging.warning("FAN_CTRL: bad data on serial line")
            except serial.serialutil.SerialException:
                self.pwm_serial_error_close()

    def process_shunt_serial(self):
        """
        Process data on SmartShunt serial line.

        This will read the current battery status, including SOC, voltage, current, etc. This overrides the data we
        receive from the BT-1 device (as it is a FAR more reliable).

        If this serial connection has disconnected (or never connected), we will attempt to reconnect it here.
        """
        self.init_shunt_serial()

        if self.shunt_serial is not None:
            try:
                packets = 0
                data = {}
                while self.shunt_serial.in_waiting > 0:
                    response = self.shunt_serial.readline()
                    if response[0:2] == b"V\t":
                        data['battery_voltage'] = round(int(response[2:-2]) / 1000, 1)

                    elif response[0:4] == b"SOC\t":
                        data['battery_soc'] = round(int(response[4:-2]) / 10, 1)

                    elif response[0:2] == b"P\t":
                        data['load_power'] = int(response[2:-2])

                    elif response[0:2] == b"I\t":
                        data['load_current'] = round(int(response[2:-2]) / 1000, 1)

                    elif response[0:4] == b"TTG\t":
                        ttg = int(response[4:-2])
                        if ttg == -1:
                            ttg = 0
                        data['time_remaining'] = -ttg

                    packets += 1
                    if packets > 20:
                        # the serial line is always sending data - if we are reading for too long, break and let the
                        # rest of the application do something
                        break

                if len(data) > 0:
                    last = self.shunt_new_metrics
                    self.shunt_new_metrics += self.power_grid.from_data(
                        data, allow_pv_data=False, allow_controller_data=False
                    )
                    if last != self.shunt_new_metrics and self.shunt_new_metrics > 0:
                        logging.debug(f"Pending updates from battery shunt: {self.shunt_new_metrics}")

                if self.shunt_new_metrics > 0:
                    # Don't hammer updates - wait a few seconds before reading again
                    if (self.shunt_last_updated is None) or (time.time() - self.shunt_last_updated > 2):
                        self.shunt_last_updated = time.time()
                        logging.debug("Dispatching updates from battery shunt")
                        self.on_metrics_updated()
                        self.shunt_new_metrics = 0

            except serial.serialutil.SerialException:
                self.shunt_serial_error_close()

    def process_config(self):
        """
        Monitors (via a Watchdog thread queue) a file that the UI app writes to.

        If the user changes settings, such as the mains configuration, we can respond to those changes.
        """
        try:
            fn = self.config_queue.get_nowait()
            if fn == get_config_file():
                self.load_config()
        except queue.Empty:
            pass

    def process_udp(self):
        """
        Process UDP traffic.

        This will be DOSA UDP payloads, notably pings and REQ_STAT messages.
        """
        msg = self.comms.receive(timeout=0.1)
        if msg is None:
            return

        if msg.msg_code == dosa.Messages.PING:
            # send PONG reply with load state
            logging.debug("PING from {}:{}".format(msg.addr[0], msg.addr[1]))
            payload = b'\x78\x01' if self.power_grid.load_state else b'\x78\x00'
            self.comms.send(self.comms.build_payload(dosa.Messages.PONG, payload), msg.addr)

        elif msg.msg_code == dosa.Messages.REQ_STAT:
            # send STATUS reply with full data dump
            logging.debug("REQ_STAT from {}:{}".format(msg.addr[0], msg.addr[1]))
            payload = struct.pack("<H", dosa.device.StatusFormat.POWER_GRID) + self.power_grid.to_bytes()
            self.comms.send(self.comms.build_payload(dosa.Messages.STATUS, payload), msg.addr)

    def spawn_bt_listener(self):
        self.bt_thread = Thread(target=self.bt_listener, args=())
        self.bt_thread.start()

    def bt_listener(self):
        """
        Run in a thread to listen to the BT-1 BLE device.

        NB: this is a blocking call.
        """
        bt1_client = Bt1Client(adapter_name=self.hci, on_data_received=self.on_data_received,
                               polling_interval=self.poll_interval)

        try:
            # Slightly misleading function name - this will also exec manager.run() and never return.
            bt1_client.connect(self.target_mac)
        except ConnectionFailedException:
            logging.error("Connection failed")
        except DisconnectedException:
            logging.error("Unexpectedly disconnected from device")
        except BleDeviceNotFoundException:
            logging.error("Device not found")
        except KeyboardInterrupt:
            logging.info("User disconnect request")
            bt1_client.disconnect()

    def on_data_received(self, app: Bt1Client, data):
        """
        Inbound data from the BLE device.
        """
        # Update the power-grid with data from the BT-1 device
        allow_bat_load = self.shunt_port is None
        if self.power_grid.from_data(data, allow_battery_data=allow_bat_load, allow_load_data=allow_bat_load) > 0:
            self.on_metrics_updated()

    def on_metrics_updated(self):
        """
        Power Grid metrics have been updated, recalculate all subsystems and write new stats.
        """
        # Dispatch a DOSA STA payload
        payload = struct.pack("<H", dosa.device.StatusFormat.POWER_GRID) + self.power_grid.to_bytes()
        logging.debug("Dispatching STA multicast")
        self.comms.send(self.comms.build_payload(dosa.Messages.STATUS, payload))

        # Recalculate power systems
        self.recalculate_mains()
        self.update_stick_colours()
        self.update_pwm_speed()

        # Write data for the GUI to read
        self.write_data_file()

    def update_stick_colours(self):
        """
        Update all glowbit sticks to the appropriate colours based on the power grid state.
        """
        if self.stick is None:
            return

        self.update_stick_mains()
        self.update_stick_pv()
        self.update_stick_battery()
        self.update_stick_load()
        self.stick.pixelsShow()

    def update_stick_mains(self):
        # Mains LED
        if self.config.index_mains is None:
            return

        if self.mains_active is None:
            mains_colour = self.config.colour_warn
        elif self.mains_active:
            mains_colour = self.config.colour_bad
        else:
            mains_colour = self.config.colour_none

        self.set_stick_colour(self.config.index_mains, mains_colour)

    def update_stick_pv(self):
        # PV LED
        logging.debug("PV: {}w, {}v".format(self.power_grid.pv_power, round(self.power_grid.pv_voltage, 1)))

        if self.config.index_pv is None:
            return

        if self.power_grid.pv_power >= self.pv_size * thresholds["pv"]["high"]:
            pv_colour = self.config.colour_special
        elif self.power_grid.pv_power >= self.pv_size * thresholds["pv"]["med"]:
            pv_colour = self.config.colour_good
        elif self.power_grid.pv_power >= self.pv_size * thresholds["pv"]["low"]:
            pv_colour = self.config.colour_warn
        else:
            pv_colour = self.config.colour_bad

        self.set_stick_colour(self.config.index_pv, pv_colour)

    def update_stick_battery(self):
        # Battery LED
        logging.debug("Battery: {}%, {}v".format(
            self.power_grid.battery_soc, round(self.power_grid.battery_voltage, 1)
        ))

        if self.config.index_bat is None:
            return

        if self.power_grid.battery_soc >= thresholds["battery"]["high"]:
            bat_colour = self.config.colour_special
        elif self.power_grid.battery_soc >= thresholds["battery"]["med"]:
            bat_colour = self.config.colour_good
        elif self.power_grid.battery_soc >= thresholds["battery"]["low"]:
            bat_colour = self.config.colour_warn
        else:
            bat_colour = self.config.colour_bad

        self.set_stick_colour(self.config.index_bat, bat_colour)

    def update_stick_load(self):
        # Load LED
        logging.debug("Load: {}w ({})".format(
            self.power_grid.load_power, "active" if self.power_grid.load_state else "inactive"
        ))

        if self.config.index_load is None:
            return

        if self.power_grid.load_power >= self.config.threshold_load_bad:
            load_colour = self.config.colour_bad
        elif self.power_grid.load_power >= self.config.threshold_load_warn:
            load_colour = self.config.colour_warn
        elif self.power_grid.load_power > 0:
            load_colour = self.config.colour_good
        else:
            load_colour = self.config.colour_special

        self.set_stick_colour(self.config.index_load, load_colour)

    def set_stick_colour(self, index, colour):
        """
        Sets all LEDs in a glowbit stick to a colour.

        This will change `StickConfig.stick_led_count` (8) LEDs at once, intended to be an entire stick in the chain.
        """
        for i in range(index, index + StickConfig.stick_led_count):
            self.stick.pixelSet(i, self.stick.rgbColour(*colour))

    def update_pwm_speed(self):
        """
        Sends a single byte down the serial line to a dedicated PWM controller. That PWM signal controls the speed of
        fans intended to cool the solar controller & inverter.
        """
        if self.pwm_port is None:
            return

        self.init_pwm_serial()

        if self.pwm_serial is None:
            return

        self.notify_ctrl_temp()

        try:
            new_speed = self.get_pwm_value()
            if new_speed != self.fan_speed:
                self.fan_speed = new_speed
                self.fan_speed = self.get_pwm_value()
                logging.debug(f"Set FAN_CTRL to {self.fan_speed}%")
                self.pwm_serial.write(self.fan_speed.to_bytes(1, byteorder="big", signed=False))

        except serial.serialutil.SerialException:
            self.pwm_serial_error_close()

    def notify_ctrl_temp(self):
        """
        Dispatches error logs if the PV controller temp is too high.

        Does nothing when the temperature is inside nominal conditions.
        """
        if (self.fan_warning_time is not None) and (time.time() - self.fan_warning_time < 15):
            return

        if self.power_grid.controller_temperature > self.config.error_temp:
            self.fan_warning_time = time.time()
            self.comms.net_log(
                LogLevel.CRITICAL,
                f"PV controller temperature critical: {self.power_grid.controller_temperature}"
            )
            logging.critical(f"PV controller temperature critical: {self.power_grid.controller_temperature}")
        elif self.power_grid.controller_temperature > self.config.warn_temp:
            self.fan_warning_time = time.time()
            self.comms.net_log(
                LogLevel.ERROR,
                f"PV controller temperature above safe levels: {self.power_grid.controller_temperature}"
            )
            logging.error(f"PV controller temperature above safe levels: {self.power_grid.controller_temperature}")

    def get_pwm_value(self):
        """
        Calculates the correct value for to send to the PWM controller.

        This value is effectively the fan speed as an integer percentage.
        """
        logging.debug("Controller temp: {}".format(self.power_grid.controller_temperature))
        return round(dosa.map_range(
            self.power_grid.controller_temperature,
            self.config.low_temp,
            self.config.high_temp,
            self.config.pwm_min,
            self.config.pwm_max
        ))

    def pwm_serial_error_close(self):
        logging.error("PWM serial communication error")
        self.pwm_serial.close()
        self.pwm_serial = None

    def shunt_serial_error_close(self):
        logging.error("SmartShunt serial communication error")
        self.shunt_serial.close()
        self.shunt_serial = None

    def write_data_file(self):
        grid_data = {
            "battery": {
                "capacity": self.bat_size,
                "soc": self.power_grid.battery_soc,
                "voltage": round(self.power_grid.battery_voltage, 2),
                "ah_remaining": round(self.bat_size * self.power_grid.battery_soc / 100, 2),
            },
            "mains": {
                "active": self.mains_active,
            },
            "pv": {
                "capacity": self.pv_size,
                "power": self.power_grid.pv_power,
                "voltage": self.power_grid.pv_voltage,
            },
            "load": {
                "power": self.power_grid.load_power,
                "current": self.power_grid.load_current,
                "ttg": self.power_grid.time_remaining,
            },
            "ctrl": {
                "temp": self.power_grid.controller_temperature,
                "fan_speed": self.fan_speed,
            },
        }

        with open(get_data_file(), 'w', encoding='utf-8') as f:
            json.dump(grid_data, f, ensure_ascii=False, indent=4)

    def recalculate_mains(self):
        if self.mains_setting > 0:
            self.recalculate_mains_override()
        else:
            self.recalculate_mains_auto()

    def recalculate_mains_override(self):
        """
        Ensure that the mains state matches the override level
        """
        if self.mains_setting == 1 and not self.mains_active:
            self.set_mains(True)

        if self.mains_setting == 2 and self.mains_active:
            self.set_mains(False)

    def recalculate_mains_auto(self):
        """
        Automatically change the mains state based on SOC and thresholds.
        """

        def set_proposed(x):
            logging.debug(f"Proposed mains relay state: {x}")
            self.mains_proposed_state = x
            self.mains_proposal_time = int(time.time())

        mains = thresholds["mains"][self.mains_config_level]
        proposed = None  # do nothing by default

        if self.power_grid.battery_soc < mains["activate"]:
            # Battery is getting low, propose enabling mains backup
            proposed = True
        elif self.power_grid.battery_soc >= mains["deactivate"]:
            # Battery SOC is high, propose turning off mains backup
            proposed = False

        if self.mains_active is None:
            # This would be the first run - set mains to whatever we decide here (no proposal = no mains)
            logging.info("Initial set for mains relay")
            self.set_mains(proposed == True)
            set_proposed(proposed)

        elif proposed is not None:
            # We need to wait for a grace period before actioning a new state proposal
            if self.mains_proposed_state != proposed:
                # Proposing a new state
                set_proposed(proposed)

            elif self.mains_active != proposed:
                # Check how long we've been in this proposed state
                time_in_proposal = int(time.time()) - self.mains_proposal_time
                if (proposed and time_in_proposal >= mains["activate_time"]) or \
                        (not proposed and time_in_proposal >= mains["deactivate_time"]):
                    # Grace expired, action proposal
                    self.set_mains(proposed)

            else:
                # Proposing what we're already doing - do nothing
                pass

    def set_mains(self, active: (bool, None)):
        """
        Set the mains backup relay to given state, and write the state to the data file.

        If active is None, a null value will be writen to the data file to indicate "still deciding" and the mains
        relay will be activated as it's the safe option until the power grid state is assessed.
        """
        logging.info(f"Set mains relay: {active}")
        self.mains_active = active
        self.write_data_file()
        self.comms.net_log(LogLevel.INFO, f"Set mains relay: {active}")

    def load_config(self):
        """
        Load the config provided by the GUI.

        You should call this on load, or when you detect changes to the config file.
        """

        def get_value(arr, key, default):
            if key in arr:
                return arr[key]
            else:
                return default

        try:
            with open(get_config_file(), 'r', encoding='utf-8') as f:
                cfg = json.load(f)
                self.mains_setting = min(max(0, int(get_value(cfg, "mains", 0))), 2)
                self.mains_config_level = min(max(0, int(get_value(cfg, "mains_opt", 1))), 2)
                logging.info(f"Updating mains config level to {self.mains_setting}:{self.mains_config_level}")
                self.comms.net_log(
                    LogLevel.INFO,
                    f"Grid updated mains configuration level to {self.mains_setting}:{self.mains_config_level}"
                )
                self.recalculate_mains()

        except (FileNotFoundError, json.decoder.JSONDecodeError):
            self.mains_config_level = 1
