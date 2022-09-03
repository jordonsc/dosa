#!/usr/bin/env python3

import logging

from dosa.exceptions import *
from dosa import Bt1Client

ble_adapter = "hci0"
target_mac = "60:98:66:db:50:c5"
poll_interval = 30  # read data interval (seconds)


def on_data_received(app: Bt1Client, data):
    logging.info("Battery SOC: %s", data["battery_percentage"])
    logging.info("Battery V:   %s", data["battery_voltage"])
    logging.info("PV Watts:    %s", data["pv_power"])

    # ST Capability Map:
    # Battery -
    # Battery SOC           battery_percentage
    # Battery Voltage       battery_voltage
    # PV -
    # Load -


def main():
    logging.basicConfig(level=logging.DEBUG)

    bt1 = Bt1Client(on_data_received=on_data_received)
    try:
        bt1.connect(target_mac)
    except ConnectionFailedException:
        logging.error("Connection failed")
    except BleDeviceNotFoundException:
        logging.error("Device not found")
    except KeyboardInterrupt:
        logging.info("User disconnect request")
        bt1.disconnect()


if __name__ == "__main__":
    main()
