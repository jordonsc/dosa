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

    def run(self, target=None):
        if target is None:
            self.run_scan()
            if len(self.devices) == 0:
                print("No devices detected")
                return

            device = self.user_select_device()
            if device is None:
                return
        else:
            device = Device(0, 0, (target, 6901))

        opt = self.user_select_opt()
        if opt == 1:
            # Debug dump
            self.exec_debug_dump(device)
        elif opt == 2:
            # BT config mode
            self.exec_config_mode(device)
        elif opt == 3:
            # Set BT password
            self.exec_device_password(device, self.get_values(["New password"]))
        elif opt == 4:
            # Set device name
            self.exec_device_name(device, self.get_values(["Device name"]))
        elif opt == 5:
            # Set wifi details
            self.exec_wifi_ap(device, self.get_values(["Wifi SSID", "Wifi Password"]))
        elif opt == 6:
            # IR sensor calibration
            self.exec_sensor_calibration(device, self.get_values(
                ["Min pixels/trigger (int)", "Single-pixel delta (float)", "Total delta (float)"]
            ))
        elif opt == 7:
            # Sonar sensor calibration
            self.exec_sonar_calibration(device, self.get_values(
                ["Trigger threshold", "Fixed calibration", "Trigger Coefficient"]
            ))
        elif opt == 8:
            # Winch driver calibration
            self.exec_door_calibration(device, self.get_values(
                ["Open distance (mm)", "Open-wait time (ms)", "Cool-down (ms)", "Close ticks (int)"]
            ))

    def exec_debug_dump(self, device):
        self.comms.send(self.comms.build_payload(dosa.Messages.DEBUG), tgt=device.address,
                        wait_for_ack=False)

        timeout = 2.0
        start_time = time.perf_counter()
        while time.perf_counter() - start_time < timeout:
            msg = self.comms.receive(timeout=timeout)

            if msg is None or msg.msg_code != dosa.Messages.LOG:
                continue

            print("[" + dosa.Messages.get_log_level(struct.unpack("<B", msg.payload[27:28])[0]) + "] " +
                  msg.payload[28:msg.payload_size].decode("utf-8"))

    def exec_config_mode(self, device):
        print("Sending Bluetooth fallback mode request..", end="")
        self.comms.send(self.comms.build_payload(dosa.Messages.REQUEST_BT_CFG_MODE), tgt=device.address,
                        wait_for_ack=True)
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
        aux[0:1] = struct.pack("<B", 0)
        aux[1:] = values[0].encode()
        self.comms.send(self.comms.build_payload(dosa.Messages.CONFIG_SETTING, aux), tgt=device.address,
                        wait_for_ack=True)
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
        aux[0:1] = struct.pack("<B", 1)
        aux[1:] = values[0].encode()
        self.comms.send(self.comms.build_payload(dosa.Messages.CONFIG_SETTING, aux), tgt=device.address,
                        wait_for_ack=True)
        print(" done")

    def exec_wifi_ap(self, device, values):
        aux = bytearray()
        aux[0:1] = struct.pack("<B", 2)

        if values is None:
            print("Clearing wifi details..", end="")
            aux[1:] = "\n".encode()
        else:
            print("Sending new wifi details..", end="")
            aux[1:] = (values[0] + "\n" + values[1]).encode()

        self.comms.send(self.comms.build_payload(dosa.Messages.CONFIG_SETTING, aux), tgt=device.address,
                        wait_for_ack=True)
        print(" done")

    def exec_sensor_calibration(self, device, values):
        aux = bytearray()
        aux[0:1] = struct.pack("<B", 3)

        if values is None:
            print("Aborting")
            exit()
        else:
            try:
                aux[1:2] = struct.pack("<B", int(values[0]))  # Min pixels
                aux[2:6] = struct.pack("<f", float(values[1]))  # Single delta
                aux[6:10] = struct.pack("<f", float(values[2]))  # Total delta
            except ValueError:
                print("Malformed calibration data, aborting")
                exit()

        print("Sending new calibration data..", end="")
        self.comms.send(self.comms.build_payload(dosa.Messages.CONFIG_SETTING, aux), tgt=device.address,
                        wait_for_ack=True)
        print(" done")

    def exec_door_calibration(self, device, values):
        aux = bytearray()
        aux[0:1] = struct.pack("<B", 4)

        if values is None:
            print("Aborting")
            exit()
        else:
            try:
                aux[1:3] = struct.pack("<H", int(values[0]))  # Open distance
                aux[3:7] = struct.pack("<L", int(values[1]))  # Open-wait time (ms)
                aux[7:11] = struct.pack("<L", int(values[2]))  # Cool-down (ms)
                aux[11:15] = struct.pack("<L", int(values[3]))  # Close ticks
            except ValueError:
                print("Malformed calibration data, aborting")
                exit()

        print("Sending new calibration data..", end="")
        self.comms.send(self.comms.build_payload(dosa.Messages.CONFIG_SETTING, aux), tgt=device.address,
                        wait_for_ack=True)
        print(" done")

    def exec_sonar_calibration(self, device, values):
        aux = bytearray()
        aux[0:1] = struct.pack("<B", 5)

        if values is None:
            print("Aborting")
            exit()
        else:
            try:
                aux[1:3] = struct.pack("<H", int(values[0]))  # Trigger threshold
                aux[3:5] = struct.pack("<H", int(values[1]))  # Fixed calibration
                aux[5:9] = struct.pack("<f", float(values[2]))  # Trigger coefficient
            except ValueError:
                print("Malformed calibration data, aborting")
                exit()

        print("Sending new calibration data..", end="")
        self.comms.send(self.comms.build_payload(dosa.Messages.CONFIG_SETTING, aux), tgt=device.address,
                        wait_for_ack=True)
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

                print("[" + str(self.device_count + 1) + "]: " +
                      msg.device_name.ljust(22) +
                      msg.addr[0].ljust(18) +
                      dosa.device.device_type_str(d.device_type).upper().ljust(20) +
                      dosa.device.device_status_str(d.device_state))

                self.device_count += 1

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
        print("[1] Request debug dump")
        print("[2] Order device into Bluetooth configuration mode")
        print("[3] Set device password")
        print("[4] Set device name")
        print("[5] Set wifi configuration")
        print("[6] Sensor calibration")
        print("[7] Sonar calibration")
        print("[8] Door calibration")

        while True:
            try:
                opt = int(input("> "))
            except ValueError:
                opt = None

            if opt is None or opt == 0:
                return None

            if 0 < opt < 9:
                print()
                return opt
            else:
                print("Invalid option")
