#!/usr/bin/env python3

import argparse
import secrets
import socket
import struct
import time

parser = argparse.ArgumentParser(description='DOSA network monitoring tool')
parser.add_argument('--map', dest='map', action='store_const',
                    const=True, default=False,
                    help='display an IR grid map with triggers')
parser.add_argument('--ignore', dest='ignore', action='store_const',
                    const=True, default=False,
                    help='ignore retry messages')
parser.add_argument('--ack', dest='ack', action='store_const',
                    const=True, default=False,
                    help='send a return ack for triggers')
args = parser.parse_args()

base_payload_size = 27
base_payload = b'\0\x01ack\0\x1bPython Script\0\0\0\0\0\0\0'

mc_ip = '239.1.1.69'
mc_port = 6901

mc_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
mc_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
mc_sock.bind((mc_ip, mc_port))

mreq = struct.pack("4sl", socket.inet_aton(mc_ip), socket.INADDR_ANY)
mc_sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)


def print_pixel(p):
    if p == 0:
        return " "

    p = p / 10
    if p > 3:
        return "#"
    elif p > 1.5:
        return "+"
    else:
        return "."


def send_ack(msg_id, tgt):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    payload = bytearray(base_payload)
    payload[0:2] = secrets.token_bytes(2)
    payload[28:30] = msg_id
    sock.sendto(payload, tgt)


last_msg_id = 0
print("DOSA Network Monitor")
while True:
    packet, addr = mc_sock.recvfrom(10240)

    if len(packet) < base_payload_size:
        continue

    msg_id = struct.unpack("<H", packet[0:2])[0]
    msg_code = packet[2:5].decode("utf-8")
    packet_size = struct.unpack("<H", packet[5:7])[0]
    device_name = packet[7:25].decode("utf-8")
    aux = ""

    if args.ignore and (msg_id == last_msg_id):
        continue

    if msg_code == "ack":
        aux = " // ACK ID: " + str(struct.unpack("<H", packet[27:29])[0])
    elif msg_code == "trg" and (last_msg_id != msg_id):
        if args.ack:
            send_ack(packet[0:2], addr)
            aux += " (replied)"
        if args.map:
            aux += "\n+--------+\n"
            index = 0
            for row in range(8):
                aux += "|"
                for col in range(8):
                    aux += print_pixel(struct.unpack("<B", packet[28 + index:29 + index])[0])
                    index += 1
                aux += "|\n"
            aux += "+--------+"
    elif msg_code == "err":
        aux = " // ERROR: " + packet[27:packet_size].decode("utf-8")
    elif msg_code == "onl":
        aux = " // ONLINE"
    elif msg_code == "bgn":
        aux = " // BEGIN SEQUENCE"
    elif msg_code == "end":
        aux = " // COMPLETE"

    t = time.strftime("%H:%M:%S", time.localtime())
    print(t + " [" + str(msg_id).rjust(5, ' ') + "] " + addr[0] + " (" + device_name + "): " + msg_code + aux)
    last_msg_id = msg_id
