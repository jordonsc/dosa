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
parser.add_argument('--ping', dest='ping', action='store',
                    help='send a ping to a given target, wait for the reply')

# Run play
parser.add_argument('-p', '--play', dest='play', action='store',
                    help='broadcast play request to any listening SecBots')

# Fire trigger
parser.add_argument('-t', '--trigger', dest='trigger', default=False, nargs='?', action='store',
                    help='fire a trigger signal; target optional, else will broadcast')

# Fire trigger
parser.add_argument('--alt', dest='alt', action='store',
                    help='convert a trigger message into an alt message with provided code')

# Request OTA update
parser.add_argument('-o', '--ota', dest='ota', default=False, nargs='?', action='store',
                    help='send an OTA update request; target optional, else will broadcast')

# Send flush command
parser.add_argument('-f', '--flush', dest='flush', default=False, nargs='?', action='store',
                    help='send cache flush command; target optional, else will broadcast')

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
        cfg.run()
    elif args.ping:
        ping = dosa.Ping(comms=comms)
        ping.run(target=args.ping)
    elif args.play:
        play = dosa.Play(comms=comms)
        play.run(args.play)
    elif args.trigger is not False:
        if args.alt is not None:
            alt = dosa.Alt(comms=comms)
            if args.trigger:
                alt.fire(args.alt, target=(args.trigger, 6901))
            else:
                alt.fire(args.alt)
        else:
            trigger = dosa.Trigger(comms=comms)
            if args.trigger:
                trigger.fire(target=(args.trigger, 6901))
            else:
                trigger.fire()
    elif args.ota is not False:
        ota = dosa.Ota(comms=comms)
        if args.ota:
            ota.dispatch(target=(args.ota, 6901))
        else:
            ota.dispatch()
    elif args.flush is not False:
        flush = dosa.Flush(comms=comms)
        if args.flush:
            flush.dispatch(target=(args.flush, 6901))
        else:
            flush.dispatch()

    else:
        snoop = dosa.Snoop(comms=comms, map=args.map, ignore=args.ignore, ack=args.ack, ignore_pings=args.noping)
        print("Listening..")
        snoop.run_snoop()

except KeyboardInterrupt:
    print("")
    sys.exit(0)
