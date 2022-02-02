import time
import dosa
import struct


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
        elif opt == 2:
            self.exec_device_password(device, self.get_values(["New password"]))
        elif opt == 3:
            self.exec_device_name(device, self.get_values(["Device name"]))
        elif opt == 4:
            self.exec_wifi_ap(device, self.get_values(["Wifi SSID", "Wifi Password"]))
        elif opt == 5:
            self.exec_calibration(device, self.get_values(
                ["Min pixels/trigger (int)", "Single-pixel delta (float)", "Total delta (float)"]
            ))

    def exec_config_mode(self, device):
        print("Sending Bluetooth fallback mode request..", end="")
        self.comms.send(self.comms.build_payload(dosa.Messages.REQUEST_BT_CFG_MODE), tgt=device.address)
        print(" done")

    def exec_device_password(self, device, values):
        if values is None:
            print("Aborting")
            exit()

        if 4 > len(values[0]) > 50:
            print("Bad password size (4-50 chars)")
            exit()

        print("Sending new password..", end="")
        aux = bytearray()
        aux[0:0] = struct.pack("<B", 0)
        aux[1:] = values[0].encode()
        self.comms.send(self.comms.build_payload(dosa.Messages.CONFIG_SETTING, aux), tgt=device.address)
        print(" done")

    def exec_device_name(self, device, values):
        if values is None:
            print("Aborting")
            exit()

        if 2 > len(values[0]) > 20:
            print("Bad device name (2-20 chars)")
            exit()

        print("Sending new device name..", end="")
        aux = bytearray()
        aux[0:0] = struct.pack("<B", 1)
        aux[1:] = values[0].encode()
        self.comms.send(self.comms.build_payload(dosa.Messages.CONFIG_SETTING, aux), tgt=device.address)
        print(" done")

    def exec_wifi_ap(self, device, values):
        aux = bytearray()
        aux[0:0] = struct.pack("<B", 2)

        if values is None:
            print("Clearing wifi details..", end="")
            aux[1:] = "\n".encode()
        else:
            print("Sending new wifi details..", end="")
            aux[1:] = (values[0] + "\n" + values[1]).encode()

        self.comms.send(self.comms.build_payload(dosa.Messages.CONFIG_SETTING, aux), tgt=device.address)
        print(" done")

    def exec_calibration(self, device, values):
        aux = bytearray()
        aux[0:0] = struct.pack("<B", 3)

        if values is None:
            print("Aborting")
            exit()
        else:
            try:
                aux[1:1] = struct.pack("<B", int(values[0]))  # Min pixels
                aux[2:6] = struct.pack("<f", float(values[1]))  # Single delta
                aux[6:10] = struct.pack("<f", float(values[2]))  # Total delta
            except ValueError:
                print("Malformed calibration data, aborting")
                exit()

        print("Sending new calibration data..", end="")
        self.comms.send(self.comms.build_payload(dosa.Messages.CONFIG_SETTING, aux), tgt=device.address)
        print(" done")

    @staticmethod
    def get_values(vals):
        """
        Retrieve an array of user inputs.
        """
        r_vals = []
        for v in vals:
            r = input(v + ": ")

            # If the first input value is blank, we'll return None
            if len(r) == 0 and len(r_vals) == 0:
                return None

            r_vals.append(r)

        return r_vals

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

                if msg is None or msg.msg_code != dosa.Messages.PONG:
                    continue

                for d in self.devices:
                    if d.address[0] == msg.addr[0]:
                        msg = None
                        break

                if msg is None:
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
        print("[2] Set device password")
        print("[3] Set device name")
        print("[4] Set wifi configuration")
        print("[5] Set sensor calibration")

        while True:
            try:
                opt = int(input("> "))
            except ValueError:
                opt = None

            if opt is None or opt == 0:
                return None

            if 0 < opt < 6:
                print()
                return opt
            else:
                print("Invalid option")
