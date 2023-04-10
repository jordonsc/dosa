from dosa.exc import DosaException


class DosaSmartException(DosaException):
    pass


class BleException(DosaSmartException):
    pass


class BleNotPoweredException(BleException):
    pass


class BleDeviceNotFoundException(BleException):
    pass


class ConnectionFailedException(BleException):
    pass


class DisconnectedException(BleException):
    pass


class Bt1Exception(DosaSmartException):
    pass
