import socket
import struct
import secrets
import time

from dosa.exc import *
from dosa.cfg import Config
from dosa.snoop import Snoop


class Messages:
    """
    3-byte message codes used by UDP comms.
    """
    ACK = b"ack"
    ERROR = b"err"
    ONLINE = b"onl"
    TRIGGER = b"trg"
    BEGIN = b"bgn"
    END = b"end"
    REQUEST_BT_CFG_MODE = b"btc"
    PING = b"pin"
    PONG = b"pon"
    CONFIG_SETTING = b"cfg"


class Message:
    def __init__(self, packet, addr):
        if len(packet) < Comms.BASE_PAYLOAD_SIZE:
            raise NotDosaPacketException("Not a valid DOSA message")

        self.payload = packet
        self.addr = addr

        self.msg_id = struct.unpack("<H", packet[0:2])[0]
        self.msg_code = packet[2:5]
        self.payload_size = struct.unpack("<H", packet[5:7])[0]
        self.device_name = packet[7:25].decode("utf-8")

    def msg_id_bytes(self):
        return self.payload[0:2]


class Comms:
    BASE_PAYLOAD_SIZE = 27
    MULTICAST_GROUP = '239.1.1.69'
    MULTICAST_PORT = 6901
    MULTICAST_MAX_HOPS = 32

    def __init__(self, device_name=b"Python Script"):
        # For binding all IPs on MC port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        self.sock.settimeout(0.01)

        # For binding MC group
        self.mc_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        self.mc_sock.settimeout(0.01)

        self.device_name = device_name

        if len(device_name) > 20:
            raise Exception("Device name cannot exceed 20 bytes")

        self.bind()

    def bind(self):
        """
        Bind the multicast port.

        This will create a bind to all IPs on the MC port, along with the MC group in two different sockets.
        """
        # to receive multicast messages -
        self.mc_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.mc_sock.bind((self.MULTICAST_GROUP, self.MULTICAST_PORT))
        self.mc_sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP,
                                struct.pack("4sl", socket.inet_aton(self.MULTICAST_GROUP), socket.INADDR_ANY))

        # to send messages, and receive direct messages -
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, self.MULTICAST_MAX_HOPS)
        self.sock.bind(('', self.MULTICAST_PORT))

    def build_payload(self, cmd, aux_data=b''):
        """
        Build a payload with a random message ID.
        """
        size = len(aux_data) + self.BASE_PAYLOAD_SIZE
        device_name_size = len(self.device_name)

        if device_name_size > 20:
            raise dosa.exc.CommsException("Device name cannot exceed 20 bytes")

        payload = bytearray()
        payload[0:2] = secrets.token_bytes(2)
        payload[2:5] = cmd
        payload[5:2] = struct.pack("<H", size)
        payload[7:7 + device_name_size] = self.device_name

        if device_name_size < 20:
            for i in range(7 + device_name_size, self.BASE_PAYLOAD_SIZE):
                payload[i:i + 1] = b'\0'

        if size > self.BASE_PAYLOAD_SIZE:
            payload[self.BASE_PAYLOAD_SIZE:] = aux_data

        return payload

    def send(self, payload, tgt=None):
        """
        Send a byte-array message to tgt.

        If tgt is None, the multicast group will be used (message broadcasted to all DOSA devices).
        """
        if tgt is None:
            tgt = (self.MULTICAST_GROUP, self.MULTICAST_PORT)
        elif type(tgt) is not tuple:
            raise Exception("Message target must be a tuple")

        self.sock.sendto(payload, tgt)

    def send_ack(self, msg_id, tgt):
        """
        Send an ACK for a given message ID back to a target.

        Message ID must be a 2-byte array.
        Target must be a tuple of (ip, port).
        """
        self.send(self.build_payload(Messages.ACK, msg_id), tgt)

    def receive(self, timeout=5.0, max_size=10240):
        """
        Wait for an return a DOSA Message object containing a received payload.
        """
        start_time = time.perf_counter()

        while timeout is None or (time.perf_counter() - start_time < timeout):
            try:
                r = self.sock.recvfrom(max_size)
                return Message(r[0], r[1])
            except (socket.timeout, NotDosaPacketException):
                pass

        return None
