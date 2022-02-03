#!/usr/bin/env python3

import argparse
import sys

import dosa

DEVICE_NAME = b"DOSA Network Tools"

# Arg parser
parser = argparse.ArgumentParser(description='DOSA network tools')

# Config mode
parser.add_argument('-c', '--cfg', dest='cfg', action='store_const',
                    const=True, default=False,
                    help='scan for devices and configure them')
parser.add_argument('-t', '--target', dest='target',
                    help='configure device at given IP address; implies -c')

# Snoop options
parser.add_argument('-m', '--map', dest='map', action='store_const',
                    const=True, default=False,
                    help='display an IR grid map with triggers')
parser.add_argument('-i', '--ignore', dest='ignore', action='store_const',
                    const=True, default=False,
                    help='ignore retry messages')
parser.add_argument('-a', '--ack', dest='ack', action='store_const',
                    const=True, default=False,
                    help='send a return ack for triggers')

# Ping
parser.add_argument('-p', '--ping', dest='ping',
                    help='send a ping to a given target, wait for the reply')

args = parser.parse_args()

# Main app
print("-- DOSA Network Monitor --")
comms = dosa.Comms(DEVICE_NAME)

try:
    if args.target:
        cfg = dosa.Config(comms=comms)
        cfg.run(target=args.target)
    elif args.cfg:
        cfg = dosa.Config(comms=comms)
        print("Scanning for devices..")
        cfg.run()
    elif args.ping:
        ping = dosa.Ping(comms=comms)
        ping.run(target=args.ping)
    else:
        snoop = dosa.Snoop(comms=comms, map=args.map, ignore=args.ignore, ack=args.ack)
        print("Listening..")
        snoop.run_snoop()

except KeyboardInterrupt:
    print("")
    sys.exit(0)
