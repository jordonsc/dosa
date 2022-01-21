#!/usr/bin/env python3

import sys
import socket

tgt_ip = sys.argv[1]
tgt_port = 6902

payload = b'\0\x01cfg\0\x1bPython Script\0\0\0\0\0\0\0'

if len(payload) != 27:
    print("Malformed payload")
    exit(1)

print("Sending request to enter config mode to " + tgt_ip + "..", end="")
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.sendto(payload, (tgt_ip, tgt_port))
print(" done")
