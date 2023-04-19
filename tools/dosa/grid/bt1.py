from threading import Timer
import logging
import time

from dosa.grid.ble import DeviceManager, Device
from dosa.grid.utilities import create_request_payload, parse_charge_controller_info, parse_set_load_response, bytes_to_int
from dosa.grid.exceptions import *

DEVICE_ID = 255
NOTIFY_CHAR_UUID = "0000fff1-0000-1000-8000-00805f9b34fb"
WRITE_CHAR_UUID = "0000ffd1-0000-1000-8000-00805f9b34fb"

READ_PARAMS = {
    'FUNCTION': 3,
    'REGISTER': 256,
    'WORDS': 34
}

WRITE_PARAMS_LOAD = {
    'FUNCTION': 6,
    'REGISTER': 266
}


class Bt1Client:
    # Default max wait time to complete the BLE scanning (seconds)
    DISCOVERY_TIMEOUT = 3

    def __init__(self, adapter_name="hci0", on_connected=None, on_data_received=None, polling_interval: int = 30):
        self.adapter_name = adapter_name
        self.connected_callback = on_connected
        self.data_received_callback = on_data_received
        self.manager = DeviceManager(adapter_name=adapter_name)
        self.interval = polling_interval
        self.device = None
        self.timer = None
        self.data = {}

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.disconnect()

    def connect(self, mac_address):
        if not self.manager.is_powered():
            raise BleNotPoweredException()

        if not self.discover(mac_address):
            raise BleDeviceNotFoundException()

        logging.info("Connecting to %s..", mac_address)

        self.device = Device(mac_address=mac_address, manager=self.manager, on_resolved=self.__on_resolved,
                             on_data=self.__on_data_received, notify_uuid=NOTIFY_CHAR_UUID, write_uuid=WRITE_CHAR_UUID)
        self.device.connect()

        return self

    def discover(self, mac_address, wait=DISCOVERY_TIMEOUT):
        if not self.manager.is_powered():
            raise BleNotPoweredException()

        self.manager.update_devices()
        self.manager.start_discovery()

        try:
            while wait > 0:
                time.sleep(1)
                for dev in self.manager.devices():
                    logging.debug("Found device: %s (%s)", dev.alias(), dev.mac_address)
                    if dev.mac_address == mac_address:
                        return True

                wait = wait - 1
        finally:
            self.manager.stop_discovery()

        return False

    def __on_resolved(self):
        if self.connected_callback is not None:
            self.connected_callback(self)

        self.poll()

    def __on_data_received(self, value):
        operation = bytes_to_int(value, 1, 1)

        if operation == 3:
            logging.debug("Received response for read operation")
            self.data = parse_charge_controller_info(value)
            if self.data_received_callback is not None:
                self.data_received_callback(self, self.data)

        elif operation == 6:
            self.data = parse_set_load_response(value)
            logging.debug("Received response for write operation")
            if self.data_received_callback is not None:
                self.data_received_callback(self, self.data)

        else:
            logging.warning("Received response for unknown operation: %s", operation)

    def poll(self):
        self.make_read_request()
        self.clear_timer()

        if self.interval > 0:
            self.timer = Timer(self.interval, self.poll)
            self.timer.start()

    def make_read_request(self):
        request = create_request_payload(
            DEVICE_ID, READ_PARAMS["FUNCTION"], READ_PARAMS["REGISTER"], READ_PARAMS["WORDS"]
        )
        self.device.characteristic_write_value(request)

    def set_load(self, value=0):
        logging.info("Setting load: %s", value)
        request = create_request_payload(
            DEVICE_ID, WRITE_PARAMS_LOAD["FUNCTION"], WRITE_PARAMS_LOAD["REGISTER"], value
        )
        self.device.characteristic_write_value(request)

    def disconnect(self):
        logging.info("BT-1 disconnecting")
        self.clear_timer()
        if self.device.is_connected():
            self.device.disconnect()

    def clear_timer(self):
        if self.timer is not None and self.timer.is_alive():
            self.timer.cancel()
