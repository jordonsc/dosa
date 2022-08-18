import dosa
import struct


class Alt:
    def __init__(self, comms=None):
        if comms is None:
            comms = dosa.Comms()

        self.comms = comms

    def fire(self, code, target=None):
        code = int(code)
        self.comms.send(self.comms.build_payload(dosa.Messages.ALT, struct.pack("<H", code)), target)
        print("Alt-trigger dispatched, code " + str(code))
