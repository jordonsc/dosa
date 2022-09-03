class DosaSmartException(Exception):
    pass


class BleException(DosaSmartException):
    pass


class BleNotPoweredException(BleException):
    pass


class BleDeviceNotFoundException(BleException):
    pass


class ConnectionFailedException(BleException):
    pass


class Bt1Exception(DosaSmartException):
    pass
