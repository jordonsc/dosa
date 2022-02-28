#!/usr/bin/env python3

import argparse
import sys

import dosa

DEVICE_NAME = b"DOSA Network Tools"

# Arg parser
parser = argparse.ArgumentParser(description='DOSA network tools')

# Config mode
parser.add_argument('-c', '--cfg', dest='cfg', default=False, nargs='?', action='store',
                    help='scan for devices and configure them; optionally private an IP address to configure')

# Ping
parser.add_argument('-p', '--ping', dest='ping', action='store',
                    help='send a ping to a given target, wait for the reply')

# Fire trigger
parser.add_argument('-t', '--trigger', dest='trigger', default=False, nargs='?', action='store',
                    help='fire a trigger signal; target optional, else will broadcast')

# Snoop options
parser.add_argument('-m', '--map', dest='map', action='store_const', const=True, default=False,
                    help='display an IR grid map or distance readouts with triggers')
parser.add_argument('-i', '--ignore', dest='ignore', action='store_const', const=True, default=False,
                    help='ignore retry messages')
parser.add_argument('-a', '--ack', dest='ack', action='store_const', const=True, default=False,
                    help='send a return ack for triggers')
parser.add_argument('-x', '--noping', dest='noping', action='store_const', const=True, default=False,
                    help='ignore ping messages')

args = parser.parse_args()

# Main app
print("-- DOSA Network Monitor --")
comms = dosa.Comms(DEVICE_NAME)

try:
    if args.cfg is not False and args.cfg is not None:
        cfg = dosa.Config(comms=comms)
        cfg.run(target=args.cfg)
    elif args.cfg is not False:
        cfg = dosa.Config(comms=comms)
        print("Scanning for devices..")
        cfg.run()
    elif args.ping:
        ping = dosa.Ping(comms=comms)
        ping.run(target=args.ping)
    elif args.trigger is not False:
        trigger = dosa.Trigger(comms=comms)
        if args.trigger:
            trigger.fire(target=(args.trigger, 6901))
        else:
            trigger.fire()

    else:
        snoop = dosa.Snoop(comms=comms, map=args.map, ignore=args.ignore, ack=args.ack, ignore_pings=args.noping)
        print("Listening..")
        snoop.run_snoop()

except KeyboardInterrupt:
    print("")
    sys.exit(0)
