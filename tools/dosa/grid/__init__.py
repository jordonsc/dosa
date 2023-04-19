import struct
import serial
import time
import logging
from threading import Thread
import json

import dosa
from dosa.grid.exceptions import *
from dosa.grid.bt1 import Bt1Client
from dosa.power import thresholds


class StickConfig:
    stick_led_count = 8
    total_led_count = 32
    fps = 15

    colour_load = (0, 100, 100)
    colour_special = (0, 30, 170)
    colour_good = (0, 150, 0)
    colour_warn = (120, 60, 0)
    colour_bad = (160, 0, 0)
    colour_none = (0, 0, 0)

    index_mains = 24
    index_bat = 16
    index_pv = 8
    index_load = 0

    threshold_load_warn = 180
    threshold_load_bad = 240

    pwm_min = 20
    pwm_max = 100
    low_temp = 25
    high_temp = 45


class GridStatus:
    def __init__(self, data: dict = None):
        self.battery_soc = 0
        self.battery_voltage = 0
        self.battery_temperature = 0

        self.pv_power = 0
        self.pv_voltage = 0
        self.pv_provided = 0

        self.load_state = False
        self.load_power = 0
        self.load_consumed = 0

        self.controller_temperature = 0

        if data is not None:
            self.from_data(data)

    def from_data(self, data: dict):
        self.battery_soc = data['battery_percentage']
        self.battery_voltage = data['battery_voltage']
        self.battery_temperature = data['battery_temperature']

        self.pv_power = data['pv_power']
        self.pv_voltage = data['pv_voltage']
        self.pv_provided = data['power_generation_today']

        self.load_state = data['load_state']
        self.load_power = data['load_power']
        self.load_consumed = data['discharging_amp_hours_today'] * 12.5

        self.controller_temperature = data['controller_temperature']

    def to_bytes(self) -> bytes:
        payload = b''
        payload += struct.pack("<B", self.battery_soc)
        payload += struct.pack("<H", round(self.battery_voltage * 10))
        payload += struct.pack("<h", self.battery_temperature)

        payload += struct.pack("<H", self.pv_power)
        payload += struct.pack("<H", round(self.pv_voltage * 10))
        payload += struct.pack("<H", self.pv_provided)

        payload += struct.pack("<B", int(self.load_state))
        payload += struct.pack("<H", self.load_power)
        payload += struct.pack("<H", round(self.load_consumed))

        payload += struct.pack("<h", self.controller_temperature)

        return payload


class PowerGrid:
    data_file = "/usr/share/power_grid.json"

    def __init__(self, tgt_mac, hci="hci0", poll_int=30, comms=None, stick=None, grid_size=1000, bat_size=500,
                 pwm_port=None):
        logging.basicConfig(level=logging.DEBUG)

        if comms is None:
            comms = dosa.Comms()
        self.comms = comms

        self.pv_size = grid_size
        self.bat_size = bat_size

        self.mains_active = None

        self.mains_proposed_state = None
        self.mains_proposal_time = None

        self.power_grid = GridStatus()

        self.hci = hci
        self.target_mac = tgt_mac
        self.poll_interval = poll_int  # read data interval (seconds)
        self.bt_thread = None
        self.stick = stick
        self.config = StickConfig()
        self.pwm_serial_port = pwm_port
        self.pwm_serial = None

        self.config.threshold_pv_special = thresholds["pv"]["high"] * grid_size
        self.config.threshold_pv_good = thresholds["pv"]["med"] * grid_size
        self.config.threshold_pv_med = thresholds["pv"]["low"] * grid_size

        self.init_lights()
        self.set_mains(None)

        if self.pwm_serial_port:
            self.init_pwm_serial()
        else:
            logging.info("No PWM serial port configured")

    def init_lights(self):
        if self.stick is None:
            logging.info("No lights configured")
            return

        logging.info("Bringing lights online..")
        self.stick.pixelsFill(self.stick.rgbColour(*self.config.colour_load))
        self.stick.pixelsShow()

    def init_pwm_serial(self):
        if not self.pwm_serial_port or self.pwm_serial is not None:
            return

        try:
            logging.info("Bringing PWM serial online..")
            self.pwm_serial = serial.Serial(self.pwm_serial_port, 9600, timeout=1)
            self.pwm_serial.reset_input_buffer()
        except serial.serialutil.SerialException:
            logging.error("Failed to bring serial online")

    def run(self):
        logging.info("Starting DOSA Power Grid Controller..")

        daemon_died = None
        self.spawn_listener()

        while True:
            # Manage the BLE listener
            if not self.bt_thread.is_alive():
                if daemon_died is not None:
                    if time.time() - daemon_died > 5:
                        logging.info("Respawning BT listener..")
                        daemon_died = None
                        self.spawn_listener()
                else:
                    logging.warning("BT listener died, respawning in 5 seconds..")
                    daemon_died = time.time()

            # Process data on serial line
            if self.pwm_serial is not None:
                try:
                    while self.pwm_serial.in_waiting > 0:
                        try:
                            logging.debug("<FAN_CTRL: {}>".format(self.pwm_serial.readline().decode('utf-8').rstrip()))
                        except UnicodeDecodeError as e:
                            logging.warning("FAN_CTRL: bad data on serial line: ", e)
                except serial.serialutil.SerialException:
                    self.serial_error_close()

            # Process UDP comms
            msg = self.comms.receive(timeout=0.1)
            if msg is None:
                continue

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

    def spawn_listener(self):
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
        self.power_grid.from_data(data)

        payload = struct.pack("<H", dosa.device.StatusFormat.POWER_GRID) + self.power_grid.to_bytes()
        logging.debug("Dispatching STA multicast")
        self.comms.send(self.comms.build_payload(dosa.Messages.STATUS, payload))

        self.recalculate_mains()
        self.update_stick_colours()
        self.update_pwm_speed()
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
        if self.pwm_serial_port is None:
            return

        self.init_pwm_serial()

        if self.pwm_serial is None:
            return

        try:
            pwm = self.get_pwm_value()
            logging.debug("Set FAN_CTRL to {}%".format(pwm))
            self.pwm_serial.write(pwm.to_bytes(1, byteorder="big", signed=False))

        except serial.serialutil.SerialException:
            self.serial_error_close()

    def get_pwm_value(self):
        """
        Calculates the correct value for to send to the PWM controller.

        This value is effectively the fan speed as an integer percentage.
        """
        logging.debug("Controller temp: {}".format(self.power_grid.controller_temperature))

        if self.power_grid.controller_temperature <= self.config.low_temp:
            return self.config.pwm_min
        elif self.power_grid.controller_temperature >= self.config.high_temp:
            return self.config.pwm_max
        else:
            pos = (self.power_grid.controller_temperature - self.config.low_temp) / \
                  (self.config.high_temp - self.config.low_temp)
            return round((pos * (self.config.pwm_max - self.config.pwm_min)) + self.config.pwm_min)

    def serial_error_close(self):
        logging.error("Serial communication error")
        self.pwm_serial.close()
        self.pwm_serial = None

    def write_data_file(self):
        # This calculation is a placeholder pending a proper shunt
        power_delta = self.power_grid.pv_power - self.power_grid.load_power

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
                "power": power_delta,
                "current": power_delta / 12.5,
            },
        }

        with open(self.data_file, 'w', encoding='utf-8') as f:
            json.dump(grid_data, f, ensure_ascii=False, indent=4)

    def recalculate_mains(self):
        def set_proposed(x):
            logging.debug("Proposed mains relay state: {}".format(x))
            self.mains_proposed_state = x
            self.mains_proposal_time = int(time.time())

        proposed = None  # do nothing by default
        if self.power_grid.battery_soc < thresholds["mains"]["activate"]:
            # Battery is getting low, propose enabling mains backup
            proposed = True
        elif self.power_grid.battery_soc >= thresholds["mains"]["deactivate"]:
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
                if (proposed and time_in_proposal >= thresholds["mains"]["activate_time"]) or \
                        (not proposed and time_in_proposal >= thresholds["mains"]["deactivate_time"]):
                    # Grace expired, action proposal
                    self.set_mains(proposed)

            else:
                # Proposing what we're already doing - do nothing
                pass

    def set_mains(self, active):
        """
        Set the mains backup relay to given state, and write the state to the data file.

        If active is None, a null value will be writen to the data file to indicate "still deciding" and the mains
        relay will be activated as it's the safe option until the power grid state is assessed.
        """
        logging.info("Set mains relay: {}".format(active))
        self.mains_active = active
        self.write_data_file()
