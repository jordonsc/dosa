import dosa
import struct
import time


class Snoop:
    def __init__(self, comms=None, ignore=False, ack=False, map=False):
        if comms is None:
            comms = dosa.Comms()
            comms.bind_multicast()

        self.last_msg_id = 0
        self.comms = comms
        self.ignore_retries = ignore
        self.auto_ack = ack
        self.print_map = map

    def run_snoop(self):
        while True:
            msg = self.comms.receive(timeout=None)
            aux = ""

            if self.ignore_retries and (msg.msg_id == self.last_msg_id):
                continue

            if msg.msg_code == dosa.Messages.ACK:
                aux = " // ACK ID: " + str(struct.unpack("<H", msg.payload[27:29])[0])
            elif msg.msg_code == dosa.Messages.TRIGGER and (msg.msg_id != self.last_msg_id):
                if self.auto_ack:
                    self.comms.send_ack(msg.msg_id_bytes(), msg.addr)
                    aux += " (replied)"
                if self.print_map:
                    aux += "\n+--------+\n"
                    index = 0
                    for row in range(8):
                        aux += "|"
                        for col in range(8):
                            aux += self.print_pixel(struct.unpack("<B", msg.payload[28 + index:29 + index])[0])
                            index += 1
                        aux += "|\n"
                    aux += "+--------+"
            elif msg.msg_code == dosa.Messages.ERROR:
                aux = " // ERROR: " + msg.payload[27:msg.payload_size].decode("utf-8")
            elif msg.msg_code == dosa.Messages.ONLINE:
                aux = " // ONLINE"
            elif msg.msg_code == dosa.Messages.BEGIN:
                aux = " // BEGIN SEQUENCE"
            elif msg.msg_code == dosa.Messages.END:
                aux = " // COMPLETE"
            elif msg.msg_code == dosa.Messages.PING:
                aux = " // PING"
            elif msg.msg_code == dosa.Messages.PONG:
                aux = " // PONG"

            t = time.strftime("%H:%M:%S", time.localtime())
            print(t + " [" + str(msg.msg_id).rjust(5, ' ') + "] " + msg.addr[0] + ":" + str(msg.addr[1]) + " (" +
                  msg.device_name + "): " + msg.msg_code.decode("utf-8") + aux)
            self.last_msg_id = msg.msg_id

    @staticmethod
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
