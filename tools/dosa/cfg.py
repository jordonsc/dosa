import time
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

        device = self.user_select_device()
        if device is None:
            return

        opt = self.user_select_opt()
        if opt == 1:
            self.exec_config_mode(device)

    def exec_config_mode(self, device):
        print("Sending config mode request..")
        self.comms.send(self.comms.build_payload(dosa.Messages.REQUEST_CFG_MODE), tgt=device.address)
        print("Done")

    def run_scan(self):
        ping = self.comms.build_payload(dosa.Messages.PING)

        retries = 2
        timeout = 1.0
        self.devices = []

        for attempt in range(retries):
            self.comms.send(ping)
            start_time = time.perf_counter()

            while time.perf_counter() - start_time < timeout:
                msg = self.comms.receive(timeout=timeout)

                if msg is None:
                    break

                if msg.msg_code != dosa.Messages.PONG:
                    continue

                for d in self.devices:
                    if d.address[0] == msg.addr[0]:
                        continue

                d = Device(
                    msg.payload[self.comms.BASE_PAYLOAD_SIZE],
                    msg.payload[self.comms.BASE_PAYLOAD_SIZE + 1],
                    msg.addr)
                self.devices.append(d)

                print("[" + str(self.device_count + 1) + "]: " + msg.device_name + " (" + msg.addr[0] + ") // " +
                      self.device_type_str(d) + "::" + self.device_status_str(d))

                self.device_count += 1

    @staticmethod
    def device_type_str(dvc):
        if dvc.device_type == 0:
            return "UNSPECIFIED"
        elif dvc.device_type == 10:
            return "MOTION SENSOR"
        elif dvc.device_type == 11:
            return "TRIP SENSOR"
        elif dvc.device_type == 50:
            return "SWITCH"
        elif dvc.device_type == 51:
            return "MOTORISED WINCH"
        else:
            return "UNKNOWN DEVICE"

    @staticmethod
    def device_status_str(dvc):
        if dvc.device_state == 0:
            return "OK"
        elif dvc.device_state == 1:
            return "ACTIVE"
        elif dvc.device_state == 10:
            return "MINOR FAULT"
        elif dvc.device_state == 11:
            return "MAJOR FAULT"
        elif dvc.device_state == 12:
            return "CRITICAL"
        else:
            return "UNKNOWN STATE"

    def user_select_device(self):
        while True:
            try:
                opt = int(input("> "))
            except ValueError:
                opt = None

            if opt is None or opt == 0:
                return None

            if 0 < opt <= len(self.devices):
                print()
                return self.devices[opt - 1]
            else:
                print("Invalid device")

    @staticmethod
    def user_select_opt():
        print("[1] Order device into Bluetooth configuration mode")
        print("[2] Set device name")
        print("[3] Set wifi configuration")
        print("[4] Set sensor calibration")

        while True:
            try:
                opt = int(input("> "))
            except ValueError:
                opt = None

            if opt is None or opt == 0:
                return None

            if 0 < opt <= 4:
                print()
                return opt
            else:
                print("Invalid option")
