#!/usr/bin/env python3

import argparse
import sys

import dosa
from dosa import glowbit
from dosa.grid import PowerGrid, StickConfig

DEVICE_NAME = b"DOSA Grid"

# Arg parser
parser = argparse.ArgumentParser(description='DOSA Power Grid Controller')

# HCI adapter name for BLE comms
parser.add_argument('-i', '--hci', dest='hci', action='store', default="hci0",
                    help='HCI adapter for BLE communications; default "hci0"')

# BT-1 polling interval (seconds)
parser.add_argument('-p', '--poll', dest='poll', action='store', default="30",
                    help='BLE polling interview in seconds; default 30')

# BT-1 Target MAC address
parser.add_argument('-m', '--mac', dest='mac', action='store',
                    help='Target Bluetooth MAC address of BT-1 device; required')
# Use a light stick
parser.add_argument('-l', '--lights', dest='lights', action='store_true',
                    help='Enables the use of a light stick')

# Transmit a PWM request via a given serial port
parser.add_argument('-s', '--pwm', dest='pwm_port', action='store',
                    help='Define the serial TTY port for sending PWM requests')

# Read from a Victron SmartShunt
parser.add_argument('-v', '--shunt', dest='shunt_port', action='store',
                    help='Define the serial TTY port for reading from a Victron SmartShunt')

# Set the size of the PV grid (in Watts)
parser.add_argument('-g', '--grid-size', dest='grid', action='store', default="1000",
                    help='Define the size of the PV grid in Watts')

# Set the size of the battery array (in Amp Hours)
parser.add_argument('-b', '--battery-size', dest='battery', action='store', default="500",
                    help='Define the size of the battery array in AH')


def run_app():
    args = parser.parse_args()

    # Main app
    print("-- DOSA Renogy Bridge --")
    comms = dosa.Comms(DEVICE_NAME)

    if args.lights:
        stick = glowbit.stick(numLEDs=StickConfig.total_led_count, rateLimitFPS=StickConfig.fps)
    else:
        stick = None

    if not args.mac:
        print("Target MAC address is required")
        sys.exit(2)

    try:
        bridge = PowerGrid(tgt_mac=args.mac, hci=args.hci, poll_int=int(args.poll), comms=comms, stick=stick,
                           grid_size=int(args.grid), bat_size=int(args.battery),
                           pwm_port=args.pwm_port, shunt_port=args.shunt_port)
        bridge.run()

        print("<app exit>")
        sys.exit(2)

    except KeyboardInterrupt:
        print("")
        sys.exit(0)


if __name__ == "__main__":
    run_app()
