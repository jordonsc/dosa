#!/usr/bin/env python3

import sys
import serial

ser = serial.Serial(
    port=sys.argv[1],
    baudrate=9600,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    timeout=1
)

try:
    while 1:
        x = ser.readline()
        if x:
            print(x.decode("utf-8"), end='')

except serial.serialutil.SerialException:
    print("\n-- Device disconnected --")

except KeyboardInterrupt:
    sys.exit(0)
