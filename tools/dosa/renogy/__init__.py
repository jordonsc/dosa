import time
import logging
from threading import Thread

import dosa
from dosa.renogy.exceptions import *
from dosa.renogy.bt1 import Bt1Client


class RenogyBridge:
    def __init__(self, tgt_mac, hci="hci0", poll_int=30, comms=None):
        logging.basicConfig(level=logging.DEBUG)

        if comms is None:
            comms = dosa.Comms()
        self.comms = comms

        self.data = None
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
                pass
            elif msg.msg_code == dosa.Messages.REQ_STAT:
                pass

            time.sleep(0.1)

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
        self.data = data
        logging.debug("Battery SOC:   %s", data["battery_percentage"])
        logging.debug("Battery V:     %s", data["battery_voltage"])
        logging.debug("PV Watts:      %s", data["pv_power"])
        logging.debug("PV V:          %s", data["pv_voltage"])
        logging.debug("PV Charge:     %s", data["power_generation_today"])
        logging.debug("Load Watts:    %s", data["load_power"])
        logging.debug("Discharge kWh: %s", data["discharging_amp_hours_today"] * data["battery_voltage"])
