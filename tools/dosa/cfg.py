import time
import struct
import dosa


class Device:
    def __init__(self, device_type, device_state, addr):
        self.device_type = device_type
        self.device_state = device_state
        self.address = addr


class Config:
    def __init__(self, comms=None):
        if comms is None:
            comms = dosa.Comms()

        self.comms = comms
        self.device_count = 0
        self.devices = []

    def run(self):
        self.run_scan()
        if len(self.devices) == 0:
            print("No devices detected")
            return

        device_id = self.user_select_device()
        if device_id is None:
            print("Exiting")
            return

    def user_select_device(self):
        return 0

    def run_scan(self):
        ping = self.comms.build_payload(dosa.Messages.PING)

        retries = 3
        timeout = 1.0
        self.devices = {}

        for attempt in range(retries):
            self.comms.send(ping)
            start_time = time.perf_counter()

            while time.perf_counter() - start_time < timeout:
                msg = self.comms.receive(timeout=timeout)

                if msg is None:
                    break

                if msg.msg_code != dosa.Messages.PONG:
                    continue

                if msg.addr[0] in self.devices:
                    continue

                d = Device(
                    msg.payload[self.comms.BASE_PAYLOAD_SIZE],
                    msg.payload[self.comms.BASE_PAYLOAD_SIZE + 1],
                    msg.addr)
                self.devices[msg.addr[0]] = d

                print("[" + str(self.device_count) + "]: " + msg.device_name + " (" + msg.addr[0] + ") " +
                      str(d.device_type) + "/" + str(d.device_state))

                self.device_count += 1

    def display_cfg_menu(self):
        print("Select option:")
        print("[0] Order device into Bluetooth configuration mode")
        print("[1] Set device name")
        print("[2] Set wifi configuration")
        print("[3] Set sensor calibration")
