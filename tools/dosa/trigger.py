import dosa


class Trigger:
    def __init__(self, comms=None):
        if comms is None:
            comms = dosa.Comms()

        self.comms = comms

    def fire(self, target=None):
        aux = bytearray(b'\x02')
        for i in range(64):
            aux += b'\x00'

        trg = self.comms.build_payload(dosa.Messages.TRIGGER, aux)
        self.comms.send(trg, target)
        print("Trigger dispatched")
