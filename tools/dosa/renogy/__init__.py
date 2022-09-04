import struct
import time
import logging
from threading import Thread

import dosa
from dosa.renogy.exceptions import *
from dosa.renogy.bt1 import Bt1Client


class PowerGrid:
    def __init__(self, data: dict = None):
        self.battery_soc = 0
        self.battery_voltage = 0
        self.battery_temperature = 0

        self.pv_power = 0
        self.pv_voltage = 0
        self.pv_provided = 0

        self.load_active = False
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

        self.load_active = bool(data['function'])
        self.load_power = data['load_power']
        self.load_consumed = data['discharging_amp_hours_today'] * 12.5

        self.controller_temperature = data['controller_temperature']

    def to_bytes(self) -> bytes:
        payload = b''
        payload += struct.pack("<B", self.battery_soc)
        payload += struct.pack("<H", self.battery_voltage * 10)
        payload += struct.pack("<h", self.battery_temperature)

        payload += struct.pack("<H", self.pv_power)
        payload += struct.pack("<H", self.pv_voltage * 10)
        payload += struct.pack("<H", self.pv_provided)

        payload += struct.pack("<B", int(self.load_active))
        payload += struct.pack("<H", self.load_power)
        payload += struct.pack("<H", round(self.load_consumed))

        payload += struct.pack("<h", self.controller_temperature)

        return payload


class RenogyBridge:
    def __init__(self, tgt_mac, hci="hci0", poll_int=30, comms=None):
        logging.basicConfig(level=logging.DEBUG)

        if comms is None:
            comms = dosa.Comms()
        self.comms = comms

        self.power_grid = PowerGrid()

        self.hci = hci
        self.target_mac = tgt_mac
        self.poll_interval = poll_int  # read data interval (seconds)
        self.bt_thread = None

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
                payload = b'0x780x01' if self.power_grid.load_active else b'0x780x00'
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
        bt1_client = Bt1Client(adapter_name=self.hci, on_data_received=self.on_data_received)

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
