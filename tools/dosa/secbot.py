import dosa
import struct
import time
import json
from boto3 import Session
from botocore.exceptions import ClientError
from dosa.tts import Tts


class SecBot:
    """
    Monitors for security alerts and errors. Vocalises them though TTS.
    """
    NO_LOG = [dosa.Messages.PING, dosa.Messages.ACK, dosa.Messages.STATUS]
    ACK_LOG = [dosa.Messages.BEGIN, dosa.Messages.END]

    def __init__(self, comms=None, voice="Emma", engine="neural"):
        if comms is None:
            comms = dosa.Comms()

        self.comms = comms
        self.tts = Tts(voice=voice, engine=engine)
        self.last_ping = 0
        self.last_heartbeat = 0
        self.devices = []
        self.settings = dosa.get_config()
        self.config = dosa.Config(self.comms)
        self.history = dosa.MessageLog()

        # Create a client using the credentials and region defined in the [dosa] section of the AWS credentials
        # file (~/.aws/credentials)
        self.session = Session(profile_name="dosa")
        self.sns = self.session.client("sns")

        # -- Settings from config file --
        # Time in seconds between heartbeats
        self.heartbeat_interval = self.get_setting(["general", "heartbeat"], 15)

        # Time in seconds between pings
        self.ping_interval = self.get_setting(["monitor", "ping"], 10)

        # Time in seconds before declaring a device offline
        self.device_timeout = self.get_setting(["monitor", "device-timeout"], 600)

        # Vocalise unresponsive device recovery
        self.report_recovery = self.get_setting(["monitor", "report-recovery"], True)

        # Log servers
        self.statsd_server = self.get_setting(["logging", "statsd"], {"server": "127.0.0.1", "port": 8125})
        self.log_server = self.get_setting(["logging", "logs"], {"server": "127.0.0.1", "port": 10518})

        # Bring up feature toggles
        self.feature = dosa.Feature(self.get_setting(["features", "server"]), self.get_setting(["features", "api-key"]))
        self.feature.log_status(self.feature.NOTIFY_OFFLINE)
        self.feature.log_status(self.feature.NOTIFY_RECONNECT)

    def get_setting(self, path, default=None):
        node = self.settings
        for p in path:
            if p in node:
                node = node[p]
            else:
                return default

        return node

    def run(self, announce=True):
        print("Security Bot online")
        self.comms.send(self.comms.build_payload(dosa.Messages.ONLINE))

        if announce:
            print("Stats server: " + self.statsd_server["server"] + ":" + str(self.statsd_server["port"]))
            print("Log server:   " + self.log_server["server"] + ":" + str(self.log_server["port"]))

            if self.tts.voice is not None:
                print("TTS voice:    " + self.tts.voice)
                self.tts.play("DOSA Security Bot online")
            else:
                print("TTS voice:    Not enabled")

        while True:
            self.do_heartbeat()
            self.check_devices()
            self.check_for_packets()

    def do_heartbeat(self):
        ct = self.get_current_time()

        # Send a heartbeat if we're stale
        if ct - self.last_heartbeat > self.heartbeat_interval:
            self.comms.send("dosa.secbot.heartbeat:1|c".encode(),
                            (self.statsd_server["server"], self.statsd_server["port"]))
            self.last_heartbeat = ct

    def check_devices(self):
        """
        Check devices on the network.

        We will periodically ping all devices. Any device that doesn't respond in a timely manner will be considered
        and incident and alerts fired.
        """
        ct = self.get_current_time()

        # Send a ping if we're stale
        if ct - self.last_ping > self.ping_interval:
            self.comms.send(self.comms.build_payload(dosa.Messages.PING))
            self.last_ping = ct

        # Allow overriding device timeout via Unleash variant -
        device_timeout = self.feature.get_variant(self.feature.NOTIFY_OFFLINE)
        if device_timeout is not None:
            device_timeout = int(device_timeout)
        else:
            device_timeout = self.device_timeout

        for d in self.devices:
            if not d.reported_unresponsive and d.is_stale(device_timeout):
                # Device is now unresponsive!
                d.reported_unresponsive = True

                # Create a log of this -
                self.comms.net_log(
                    dosa.LogLevel.ERROR,
                    "Device unresponsive: " + d.device_name + " at " + d.address[0] + ":" + str(d.address[1])
                )

                # Vocalise an alert -
                if self.tts is not None:
                    self.tts.play("Alert, " + d.device_name + " is not responding")

                # Raise an incident -
                if self.feature.is_enabled(dosa.Feature.NOTIFY_OFFLINE):
                    self.alert(
                        d.device_name, d.device_name + " is not responding",
                        category=dosa.AlertCategory.NETWORK,
                        level=dosa.LogLevel.as_string(dosa.LogLevel.ERROR)
                    )

    def check_for_packets(self):
        """
        Check for and process incoming traffic.
        """
        packet = self.comms.receive(timeout=0.1)
        if packet is None:
            return

        device = dosa.Device(msg=packet)
        if self.history.validate(device, packet.msg_id):
            return

        msg = ""

        if packet.msg_code in self.ACK_LOG:
            # These commands we'll ack but otherwise won't do anything special with them
            self.comms.send_ack(packet.msg_id_bytes(), packet.addr)
            self.log(packet)

        elif packet.msg_code in self.NO_LOG:
            # Don't log pings, acks, etc
            pass

        elif packet.msg_code == dosa.Messages.LOG:
            # For log messages, we'll hunt down any error or critical messages and raise alerts
            log_level = struct.unpack("<B", packet.payload[27:28])[0]
            log_message = packet.payload[28:packet.payload_size].decode("utf-8")
            aux = " | " + dosa.LogLevel.as_string(log_level) + " | " + log_message

            # For ALL log messages, we will ack and forward to the log server
            self.log(packet, aux)
            self.comms.send_ack(packet.msg_id_bytes(), packet.addr)

            # But we raise incidents for, or vocalise own error messages
            if device.device_name == self.comms.device_name.decode("utf-8"):
                return

            if log_level == dosa.LogLevel.CRITICAL:
                msg = "Warning, " + packet.device_name + " critical. " + log_message + "."

            if msg:
                # Raise an incident for this log message
                self.alert(
                    packet.device_name,
                    msg,
                    category=dosa.AlertCategory.NETWORK,
                    level=dosa.LogLevel.as_string(log_level)
                )

        elif packet.msg_code == dosa.Messages.SEC:
            # Security messages require an alert raised
            sec_level = struct.unpack("<B", packet.payload[27:28])[0]
            aux = " | " + dosa.SecurityLevel.as_string(sec_level)
            self.log(packet, aux)
            self.comms.send_ack(packet.msg_id_bytes(), packet.addr)

            if sec_level == dosa.SecurityLevel.ALERT:
                msg = "Security alert, " + packet.device_name + ", activity"
            elif sec_level == dosa.SecurityLevel.BREACH:
                msg = "Security alert, " + packet.device_name + ", breach"
            elif sec_level == dosa.SecurityLevel.TAMPER:
                msg = "Security alert, " + packet.device_name + ", tamper warning"
            elif sec_level == dosa.SecurityLevel.PANIC:
                msg = "Security alert, " + packet.device_name + ", panic alarm triggered"

            self.alert(packet.device_name, msg, category=dosa.AlertCategory.SECURITY,
                       level=dosa.SecurityLevel.as_string(sec_level))

        elif packet.msg_code == dosa.Messages.FLUSH:
            # Net flush - we'll dump our device registry in-line with flush protocol
            msg = "Network flush initiated by " + packet.device_name
            self.log(packet)
            self.devices.clear()
            self.last_ping = 0

        elif packet.msg_code == dosa.Messages.TRIGGER:
            # Trigger messages may contain information about the trigger parameters, decode them and add to log msg
            trigger_type = struct.unpack("<B", packet.payload[27:28])[0]
            if trigger_type == 0:
                self.log(packet, " | UNKNOWN")
            elif trigger_type == 1:
                self.log(packet, " | BUTTON")
            elif trigger_type == 2:
                self.log(packet, " | SENSOR")
            elif trigger_type == 3:
                # Ranging sensor, show distances
                dist_prev = struct.unpack("<H", packet.payload[28:30])[0]
                dist_new = struct.unpack("<H", packet.payload[30:32])[0]
                self.log(packet, " | RANGE | " + str(dist_prev) + " | " + str(dist_new))
            elif trigger_type == 4:
                # IR grid map - could log some data here, but probably too much for a single-line logfile
                self.log(packet, " | MAP")
            elif trigger_type == 100:
                self.log(packet, " | AUTO")

        elif packet.msg_code == dosa.Messages.PLAY:
            # A device has requested a play be run, we're responsible for that
            self.comms.send_ack(packet.msg_id_bytes(), packet.addr)
            play = packet.payload[27:packet.payload_size].decode("utf-8")
            self.log(packet, " | " + play)
            self.run_play(play)

        elif packet.msg_code == dosa.Messages.PONG:
            # Ignore ping/pong messages in logs, but register/update device details when we see a pong
            match = False
            for d in self.devices:
                if d.address == device.address:
                    match = True
                    d.pong()
                    if d.reported_unresponsive:
                        # Device recovery
                        d.reported_unresponsive = False
                        self.comms.net_log(dosa.LogLevel.WARNING, "Device recovery: " + d.device_name)

                        # Verbal recovery notice
                        if self.report_recovery:
                            msg = "Notice, " + d.device_name + " is back online"

                        # Alert recovery notice
                        if self.feature.is_enabled(dosa.Feature.NOTIFY_RECONNECT):
                            self.alert(
                                d.device_name, d.device_name + " has recovered",
                                category=dosa.AlertCategory.NETWORK,
                                level=dosa.LogLevel.as_string(dosa.LogLevel.INFO)
                            )
                    break

            if not match:
                device.device_type = packet.payload[self.comms.BASE_PAYLOAD_SIZE]
                device.device_state = packet.payload[self.comms.BASE_PAYLOAD_SIZE + 1]
                self.devices.append(device)
                print("Found device: " + device.device_name)

        else:
            self.log(packet)

        if msg:
            print(msg)
            self.tts.play(msg, wait=False)

    def log(self, msg, aux=""):
        """
        Send a log to the log server.
        """
        t = time.strftime("%H:%M:%S", time.localtime())
        payload = t + " [" + str(msg.msg_id).rjust(5, ' ') + "] " + msg.addr[0] + ":" + str(
            msg.addr[1]) + " (" + msg.device_name + "): " + msg.msg_code.decode("utf-8").upper() + aux
        self.comms.send(payload.encode(), (self.log_server["server"], self.log_server["port"]))

    def alert(self, device, msg, category, level, tags=None):
        """
        Raise an incident/notify alert end-points.
        """
        if "alerts" not in self.settings:
            return

        if tags is None:
            tags = {"device": device, "category": category, "level": level}
        else:
            tags["device"] = device
            tags["category"] = category
            tags["level"] = level

        if category not in self.settings["alerts"]:
            return

        sns_tags = {}
        for key, value in tags.items():
            sns_tags[key] = {"DataType": "String", "StringValue": value}

        for arn in self.settings["alerts"][category]:
            try:
                self.sns.publish(
                    TargetArn=arn,
                    Message=json.dumps({'default': msg}),
                    MessageStructure='json',
                    MessageAttributes=sns_tags,
                )

                print(category + " alert dispatched to " + arn)
                self.comms.net_log(
                    dosa.LogLevel.WARNING,
                    category + " alert dispatched to " + arn
                )
            except ClientError:
                self.comms.net_log(
                    dosa.LogLevel.ERROR,
                    "SecBot failed to sent alert for device " + device
                )

    def run_play(self, play):
        """
        Execute a playbook.
        """
        if "plays" not in self.settings or play not in self.settings["plays"] or "actions" not in \
                self.settings["plays"][play]:
            return False

        print("run play: " + play)

        for action in self.settings["plays"][play]["actions"]:
            self.run_action(action)

    def run_action(self, action):
        """
        Execute an action in a playbook.
        """
        if "action" not in action:
            return False

        act = action["action"]
        print("run action: " + act)

        if act == "announce" and "value" in action:
            self.run_action_announce(action["value"])
        elif act == "set-lock" and "devices" in action and "value" in action:
            self.run_action_set_lock(action["devices"], action["value"])

    def run_action_announce(self, msg):
        self.tts.play(msg)

    def run_action_set_lock(self, devices, value):
        if value < 0 or value > 3:
            self.comms.net_log(dosa.LogLevel.WARNING, "Bad lock state in play: " + str(value))
            return

        for device in devices:
            found = False
            for reg_device in self.devices:
                if reg_device.device_name == device:
                    found = True
                    if self.config.exec_lock_state(reg_device, value):
                        self.comms.net_log(
                            dosa.LogLevel.INFO,
                            "Set " + device + " to lock state " + dosa.LockLevel.as_string(value)
                        )
                    else:
                        msg = "Failed to set " + device + " to lock state " + dosa.LockLevel.as_string(value)
                        self.comms.net_log(dosa.LogLevel.ERROR, msg)
                        self.tts.play("Error executing play: " + msg)

            if not found:
                self.comms.net_log(dosa.LogLevel.WARNING, "Unknown device in play: " + device)
                self.tts.play("Unknown device in play: " + device)

    @staticmethod
    def get_current_time():
        return int(time.time())
