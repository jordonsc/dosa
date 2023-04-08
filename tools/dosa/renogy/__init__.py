import struct
import serial
import time
import logging
from threading import Thread

import dosa
from dosa.renogy.exceptions import *
from dosa.renogy.bt1 import Bt1Client


class StickConfig:
    stick_led_count = 8
    total_led_count = 24
    fps = 15

    colour_load = (0, 100, 100)
    colour_special = (0, 30, 170)
    colour_good = (0, 150, 0)
    colour_warn = (120, 60, 0)
    colour_bad = (160, 0, 0)

    index_pv = 0
    index_bat = 8
    index_load = 16

    # These values (PV only) are coefficients of the grid-size, and will be converted on RenogyBridge init
    threshold_pv_special = 0.8
    threshold_pv_good = 0.35
    threshold_pv_med = 0.05

    threshold_bat_special_v = 13.7
    threshold_bat_special_soc = 100
    threshold_bat_good = 95
    threshold_bat_med = 85
    threshold_load_warn = 180
    threshold_load_bad = 240

    pwm_min = 20
    pwm_max = 100
    low_temp = 25
    high_temp = 45


class PowerGrid:
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
        self.load_consumed = data['discharging_amp_hours_today'] * 13

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


class RenogyBridge:
    def __init__(self, tgt_mac, hci="hci0", poll_int=30, comms=None, stick=None, grid_size=1000, pwm_port=None):
        logging.basicConfig(level=logging.DEBUG)

        if comms is None:
            comms = dosa.Comms()
        self.comms = comms

        self.power_grid = PowerGrid()

        self.hci = hci
        self.target_mac = tgt_mac
        self.poll_interval = poll_int  # read data interval (seconds)
        self.bt_thread = None
        self.stick = stick
        self.config = StickConfig()
        self.pwm_serial_port = pwm_port
        self.pwm_serial = None

        self.config.threshold_pv_special = StickConfig.threshold_pv_special * grid_size
        self.config.threshold_pv_good = StickConfig.threshold_pv_good * grid_size
        self.config.threshold_pv_med = StickConfig.threshold_pv_med * grid_size

        self.init_lights()

        if self.pwm_serial_port:
            self.init_pwm_serial()
        else:
            logging.info("No PWM serial port configured")

    def init_lights(self):
        if self.stick is None:
            logging.info("No lights configured")
            return

        logging.info("Bringing lights online..")
        self.stick.blankDisplay()
        for i in range(self.config.total_led_count):
            self.stick.pixelSet(i, self.stick.rgbColour(*self.config.colour_load))
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
        logging.info("Starting DOSA Renogy Bridge..")

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
                            logging.debug("Serial: " + self.pwm_serial.readline().decode('utf-8').rstrip())
                        except UnicodeDecodeError as e:
                            logging.warning("Bad data on serial line: ", e)
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
        bt1_client = Bt1Client(adapter_name=self.hci, on_data_received=self.on_data_received,
                               polling_interval=self.poll_interval)

        try:
            bt1_client.connect(self.target_mac)
        except ConnectionFailedException:
            logging.error("Connection failed")
        except BleDeviceNotFoundException:
            logging.error("Device not found")
        except KeyboardInterrupt:
            logging.info("User disconnect request")
            bt1_client.disconnect()

    def on_data_received(self, app: Bt1Client, data):
        self.power_grid.from_data(data)

        payload = struct.pack("<H", dosa.device.StatusFormat.POWER_GRID) + self.power_grid.to_bytes()
        self.comms.send(self.comms.build_payload(dosa.Messages.STATUS, payload))

        self.update_stick_colours()
        self.update_pwm_speed()

    def update_stick_colours(self):
        if self.stick is None:
            return

        # PV
        if self.power_grid.pv_power >= self.config.threshold_pv_special:
            pv_colour = self.config.colour_special
        elif self.power_grid.pv_power >= self.config.threshold_pv_good:
            pv_colour = self.config.colour_good
        elif self.power_grid.pv_power > self.config.threshold_pv_med:
            pv_colour = self.config.colour_warn
        else:
            pv_colour = self.config.colour_bad

        self.set_stick_colour(self.config.index_pv, pv_colour)

        # Battery
        if self.power_grid.battery_soc >= self.config.threshold_bat_special_soc and \
                self.power_grid.battery_voltage >= self.config.threshold_bat_special_v:
            bat_colour = self.config.colour_special
        elif self.power_grid.battery_soc >= self.config.threshold_bat_good:
            bat_colour = self.config.colour_good
        elif self.power_grid.battery_soc >= self.config.threshold_bat_med:
            bat_colour = self.config.colour_warn
        else:
            bat_colour = self.config.colour_bad

        self.set_stick_colour(self.config.index_bat, bat_colour)

        # Load
        if self.power_grid.load_power >= self.config.threshold_load_bad:
            load_colour = self.config.colour_bad
        elif self.power_grid.load_power >= self.config.threshold_load_warn:
            load_colour = self.config.colour_warn
        elif self.power_grid.load_power > 0:
            load_colour = self.config.colour_good
        else:
            load_colour = self.config.colour_special

        self.set_stick_colour(self.config.index_load, load_colour)

        self.stick.pixelsShow()

    def set_stick_colour(self, index, colour):
        for i in range(index, index + 8):
            self.stick.pixelSet(i, self.stick.rgbColour(*colour))

    def update_pwm_speed(self):
        if self.pwm_serial_port is None:
            return

        self.init_pwm_serial()

        if self.pwm_serial is None:
            return

        try:
            pwm = self.get_pwm_value()
            logging.debug("Set PWM to {}".format(pwm))
            self.pwm_serial.write(pwm.to_bytes(1, byteorder="big", signed=False))

        except serial.serialutil.SerialException:
            self.serial_error_close()

    def get_pwm_value(self):
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
