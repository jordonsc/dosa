import gatt
import logging
import time

from dosa.renogy.exceptions import *


class DeviceManager(gatt.DeviceManager):
    def __init__(self, adapter_name):
        super().__init__(adapter_name)
        logging.debug("Adapter status: %s", "powered" if self.is_powered() else "unpowered")

    def is_powered(self) -> bool:
        return self.is_adapter_powered

    def start_discovery(self):
        super().start_discovery()

    def stop_discovery(self):
        super().stop_discovery()

    def update_devices(self):
        super().update_devices()

    def devices(self):
        return super().devices()


class Device(gatt.Device):
    def __init__(self, mac_address, manager, on_resolved, on_data, notify_uuid, write_uuid):
        # To avoid warnings from the Gatt library's stubs
        self.manager = None
        self.mac_address = None
        self.services = []
        self.write_characteristic = None
        self.writing = False

        super().__init__(mac_address=mac_address, manager=manager)

        self.data_callback = on_data
        self.resolved_callback = on_resolved

        self.notify_char_uuid = notify_uuid
        self.write_char_uuid = write_uuid

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.disconnect()

    def connect(self):
        super().connect()
        self.manager.run()
        return self

    def disconnect(self):
        super().disconnect()
        self.manager.stop()

    def connect_succeeded(self):
        super().connect_succeeded()
        logging.info("Connected to %s", self.mac_address)

    def connect_failed(self, error):
        super().connect_failed(error)
        logging.warning("Connection to device %s failed: %s", self.mac_address, str(error))
        raise ConnectionFailedException(str(error))

    def disconnect_succeeded(self):
        super().disconnect_succeeded()
        logging.info("Disconnected from %s", self.mac_address)

    def services_resolved(self):
        super().services_resolved()

        logging.debug("Resolved services for %s", self.mac_address)
        for service in self.services:
            for characteristic in service.characteristics:
                if characteristic.uuid == self.notify_char_uuid:
                    characteristic.enable_notifications()
                    logging.debug("Subscribed to notification: %s", characteristic.uuid)
                if characteristic.uuid == self.write_char_uuid:
                    self.write_characteristic = characteristic
                    logging.debug("Found write characteristic: %s", characteristic.uuid)

        self.resolved_callback()

    def descriptor_read_value_failed(self, descriptor, error):
        super().descriptor_read_value_failed(descriptor, error)
        logging.error('descriptor_value_failed')

    def characteristic_enable_notifications_succeeded(self, characteristic):
        super().characteristic_enable_notifications_succeeded(characteristic)
        logging.debug('characteristic_enable_notifications_succeeded')

    def characteristic_enable_notifications_failed(self, characteristic, error):
        super().characteristic_enable_notifications_failed(characteristic, error)
        logging.error('characteristic_enable_notifications_failed')

    def characteristic_value_updated(self, characteristic, value):
        super().characteristic_value_updated(characteristic, value)
        self.data_callback(value)

    def characteristic_write_value(self, value):
        logging.debug("Send data: %s", value)
        self.write_characteristic.write_value(value)
        self.writing = value

    def characteristic_write_value_succeeded(self, characteristic):
        super().characteristic_write_value_succeeded(characteristic)
        logging.debug('characteristic_write_value_succeeded')
        self.writing = False

    def characteristic_write_value_failed(self, characteristic, error):
        super().characteristic_write_value_failed(characteristic, error)
        logging.debug('characteristic_write_value_failed')
        if error == "In Progress" and self.writing is not False:
            time.sleep(0.1)
            self.characteristic_write_value(self.writing, characteristic)
        else:
            self.writing = False

    def alias(self):
        alias = super().alias()

        if alias:
            return alias.strip()
        else:
            return None
