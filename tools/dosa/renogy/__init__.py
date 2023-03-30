import struct
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

    threshold_pv_special = 450
    threshold_pv_good = 250
    threshold_pv_med = 25
    threshold_bat_special_v = 13.7
    threshold_bat_special_soc = 100
    threshold_bat_good = 95
    threshold_bat_med = 85
    threshold_load_warn = 180
    threshold_load_bad = 240


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
    def __init__(self, tgt_mac, hci="hci0", poll_int=30, comms=None, stick=None):
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

        self.init_lights()

    def init_lights(self):
        if self.stick is None:
            return

        self.stick.blankDisplay()
        for i in range(StickConfig.total_led_count):
            self.stick.pixelSet(i, self.stick.rgbColour(*StickConfig.colour_load))
            self.stick.pixelsShow()

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

    def update_stick_colours(self):
        if self.stick is None:
            return

        # PV
        if self.power_grid.pv_power >= StickConfig.threshold_pv_special:
            pv_colour = StickConfig.colour_special
        elif self.power_grid.pv_power >= StickConfig.threshold_pv_good:
            pv_colour = StickConfig.colour_good
        elif self.power_grid.pv_power > StickConfig.threshold_pv_med:
            pv_colour = StickConfig.colour_warn
        else:
            pv_colour = StickConfig.colour_bad

        self.set_stick_colour(StickConfig.index_pv, pv_colour)

        # Battery
        if self.power_grid.battery_soc >= StickConfig.threshold_bat_special_soc and \
                self.power_grid.battery_voltage >= StickConfig.threshold_bat_special_v:
            bat_colour = StickConfig.colour_special
        elif self.power_grid.battery_soc >= StickConfig.threshold_bat_good:
            bat_colour = StickConfig.colour_good
        elif self.power_grid.battery_soc >= StickConfig.threshold_bat_med:
            bat_colour = StickConfig.colour_warn
        else:
            bat_colour = StickConfig.colour_bad

        self.set_stick_colour(StickConfig.index_bat, bat_colour)

        # Load
        if self.power_grid.load_power >= StickConfig.threshold_load_bad:
            load_colour = StickConfig.colour_bad
        elif self.power_grid.load_power >= StickConfig.threshold_load_warn:
            load_colour = StickConfig.colour_warn
        elif self.power_grid.load_power > 0:
            load_colour = StickConfig.colour_good
        else:
            load_colour = StickConfig.colour_special

        self.set_stick_colour(StickConfig.index_load, load_colour)

        self.stick.pixelsShow()

    def set_stick_colour(self, index, colour):
        for i in range(index, index + 8):
            self.stick.pixelSet(i, self.stick.rgbColour(*colour))
