import dosa


class Trigger:
    def __init__(self, comms=None):
        if comms is None:
            comms = dosa.Comms()

        self.comms = comms

    def fire(self, target=None):
        aux = bytearray(b'\0')
        for i in range(65):
            aux += b'\xFF'

        trg = self.comms.build_payload(dosa.Messages.TRIGGER, aux)
        self.comms.send(trg, target)
        print("Trigger dispatched")
