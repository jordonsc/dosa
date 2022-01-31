import time
import dosa


class Config:
    def __init__(self, comms=None):
        if comms is None:
            comms = dosa.Comms()

        self.comms = comms
        self.device_count = 0

    def run_scan(self):
        self.comms.send(self.comms.build_payload(dosa.Messages.PING))

        timeout = 3.0
        start_time = time.perf_counter()

        while time.perf_counter() - start_time < timeout:
            msg = self.comms.receive(timeout=timeout)

            if msg is None:
                break

            if msg.msg_code != dosa.Messages.PONG:
                continue

            print("[" + str(self.device_count) + "]: " + msg.device_name + " (" + msg.addr[0] + ")")
            self.device_count += 1
