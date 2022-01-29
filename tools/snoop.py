#!/usr/bin/env python3

import struct
import socket
import time

mc_ip = '239.1.1.69'
mc_port = 6901

mc_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
mc_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
mc_sock.bind((mc_ip, mc_port))

mreq = struct.pack("4sl", socket.inet_aton(mc_ip), socket.INADDR_ANY)
mc_sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)


print("Snooping..")
while True:
    packet, addr = mc_sock.recvfrom(10240)

    if len(packet) < 27:
        continue

    msg_id = struct.unpack("<H", packet[0:2])[0]
    msg_code = packet[2:5].decode("utf-8")
    device_name = packet[7:25].decode("utf-8")
    aux = ""

    if msg_code == "ack":
        aux = " // ACK ID: " + str(struct.unpack("<H", packet[27:29])[0])

    t = time.strftime("%H:%M:%S", time.localtime())
    print(t + " [" + str(msg_id).rjust(5, ' ') + "] " + addr[0] + " (" + device_name + "): " + msg_code + aux)
