#!/usr/bin/env python3

import argparse
import sys

import dosa
from dosa.renogy import RenogyBridge, StickConfig
from dosa import glowbit

DEVICE_NAME = b"DOSA Renogy Bridge"

# Arg parser
parser = argparse.ArgumentParser(description='DOSA Renogy Bridge')

# HCI adapter name
parser.add_argument('-i', '--hci', dest='hci', action='store', default="hci0",
                    help='HCI adapter name; default "hci0"')

# HCI adapter name
parser.add_argument('-p', '--poll', dest='poll', action='store', default="30",
                    help='Polling interview in seconds; default 30')

# Target MAC address
parser.add_argument('-m', '--mac', dest='mac', action='store',
                    help='Target MAC address of BT-1 device; required')

# Use a light stick
parser.add_argument('-l', '--lights', dest='lights', action='store_true',
                    help='Enables the use of a light stick')

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
    bridge = RenogyBridge(tgt_mac=args.mac, hci=args.hci, poll_int=int(args.poll), comms=comms, stick=stick)
    bridge.run()

except KeyboardInterrupt:
    print("")
    sys.exit(0)
