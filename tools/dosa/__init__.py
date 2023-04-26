import pathlib

from dosa.exc import *
from dosa.comms import Messages, Message, Comms
from dosa.cfg import Config
from dosa.snoop import Snoop
from dosa.ping import Ping
from dosa.alt import Alt
from dosa.trigger import Trigger
from dosa.ota import Ota
from dosa.flush import Flush
from dosa.play import Play
from dosa.device import DeviceType, DeviceStatus, Device
from UnleashClient import UnleashClient


class Feature:
    NOTIFY_OFFLINE = "notify-offline"
    NOTIFY_RECONNECT = "notify-reconnected"

    def __init__(self, unleash_url: str = None, api_key: str = None):
        if unleash_url is None:
            print("Unleash service not available")
            self.unleash = None
        else:
            print("Unleash URL: " + unleash_url)
            self.unleash = UnleashClient(
                url=unleash_url,
                app_name="dosa-secbot",
                custom_headers={'Authorization': api_key}
            )

            self.unleash.initialize_client()

    def is_enabled(self, feature: str):
        if self.unleash is not None:
            return self.unleash.is_enabled(feature, fallback_function=self.feature_fallback)
        else:
            return True

    def get_variant(self, feature: str):
        if self.unleash is not None:
            var = self.unleash.get_variant(feature)
            if var['enabled'] and var['name'] != "disabled":
                return var['payload']['value']
            else:
                return None
        else:
            return None

    def log_status(self, feature: str):
        print("feature [" + feature + "]: ", end="")
        if self.is_enabled(feature):
            print("enabled")
        else:
            print("disabled")

    @staticmethod
    def feature_fallback(feature_name: str, context: dict) -> bool:
        print("feature fallback: " + feature_name)
        return True


class LogLevel:
    DEBUG = 10
    INFO = 20
    STATUS = 30
    WARNING = 40
    ERROR = 50
    CRITICAL = 60

    @staticmethod
    def as_string(log_level):
        if log_level == LogLevel.DEBUG:
            return "DEBUG"
        elif log_level == LogLevel.INFO:
            return "INFO"
        elif log_level == LogLevel.STATUS:
            return "STATUS"
        elif log_level == LogLevel.WARNING:
            return "WARNING"
        elif log_level == LogLevel.ERROR:
            return "ERROR"
        elif log_level == LogLevel.CRITICAL:
            return "CRITICAL"
        else:
            return "UNKNOWN"


class SecurityLevel:
    ALERT = 0
    BREACH = 1
    TAMPER = 2
    PANIC = 3

    @staticmethod
    def as_string(sec_level):
        if sec_level == SecurityLevel.ALERT:
            return "ALERT"
        elif sec_level == SecurityLevel.BREACH:
            return "BREACH"
        elif sec_level == SecurityLevel.TAMPER:
            return "TAMPER"
        elif sec_level == SecurityLevel.PANIC:
            return "PANIC"
        else:
            return "UNKNOWN"


class LockLevel:
    UNLOCKED = 0
    LOCKED = 1
    ALERT = 2
    BREACH = 3

    @staticmethod
    def as_string(lock_level):
        if lock_level == LockLevel.UNLOCKED:
            return "UNLOCKED"
        elif lock_level == LockLevel.LOCKED:
            return "LOCKED"
        elif lock_level == LockLevel.ALERT:
            return "ALERT"
        elif lock_level == LockLevel.BREACH:
            return "BREACH"
        else:
            return "UNKNOWN"


class AlertCategory:
    SECURITY = "Security"
    NETWORK = "Network"


class MessageLog:
    def __init__(self, max_history=25):
        self.history = []
        self.max_history = max_history

    def validate(self, device, msg_id):
        if self.is_registered(device, msg_id):
            return True
        else:
            self.add_device(device, msg_id)
            return False

    def is_registered(self, device, msg_id):
        for d, m in self.history:
            if device.address == d.address and m == msg_id:
                return True

        return False

    def add_device(self, device, msg_id):
        if device.address is None:
            return

        if len(self.history) == self.max_history:
            self.history.pop(0)

        self.history.append((device, msg_id))


def get_config_file():
    home_file = pathlib.Path(pathlib.Path.home(), ".dosa", "config")
    system_file = pathlib.Path("/", "etc", "dosa", "config")

    if home_file.exists() and home_file.is_file():
        return home_file
    elif system_file.exists() and system_file.is_file():
        return system_file
    else:
        return None


def map_range(value, in_low, in_high, out_low, out_high):
    if value <= in_low:
        return out_low
    elif value >= in_high:
        return out_high
    else:
        pos = (value - in_low) / (in_high - in_low)
        return (pos * (out_high - out_low)) + out_low
