#!/usr/bin/env python3

import struct
import socket
import secrets

mc_ip = '239.1.1.69'
mc_port = 6901

base_payload = b'\0\x01ack\0\x1bPython Script\0\0\0\0\0\0\0'

mc_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
mc_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
mc_sock.bind((mc_ip, mc_port))

mreq = struct.pack("4sl", socket.inet_aton(mc_ip), socket.INADDR_ANY)
mc_sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)


def send_ack(msg_id, tgt):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    payload = bytearray(base_payload)
    payload[0:2] = secrets.token_bytes(2)
    payload[28:30] = msg_id
    sock.sendto(payload, tgt)


print("Listening for messages..")
while True:
    packet, addr = mc_sock.recvfrom(10240)

    if len(packet) < 27:
        continue

    msg_id = packet[0:2]
    msg_code = packet[2:5].decode("utf-8")
    device_name = packet[5:25].decode("utf-8")

    print("Message: '" + msg_code + "' from " + device_name + " (" + addr[0] + ")..", end="")
    send_ack(msg_id, addr)
    print(" ack'd")
