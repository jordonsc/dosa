import dosa
import struct
import time
import requests
from dosa.tts import Tts


class SecBot:
    """
    Monitors for security alerts and errors. Vocalises them though TTS.
    """

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

        # -- Settings from config file --
        # Time in seconds between heartbeats
        self.heartbeat_interval = self.get_setting(["general", "heartbeat"], 15)

        # Time in seconds between pings
        self.ping_interval = self.get_setting(["monitor", "ping"], 10)

        # Time in seconds before declaring a device offline
        self.device_timeout = self.get_setting(["monitor", "device-timeout"], 60)

        # Vocalise unresponsive device recovery
        self.report_recovery = self.get_setting(["monitor", "report-recovery"], True)

        # Log servers
        self.statsd_server = self.get_setting(["logging", "statsd"], {"server": "127.0.0.1", "port": 8125})
        self.log_server = self.get_setting(["logging", "logs"], {"server": "127.0.0.1", "port": 10518})

    def get_setting(self, path, default):
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
            self.tts.play("DOSA Security Bot online")

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

        for d in self.devices:
            if not d.reported_unresponsive and d.is_stale(self.device_timeout):
                # Device is now unresponsive!
                d.reported_unresponsive = True

                # Create a log of this -
                self.comms.net_log(
                    dosa.LogLevel.ERROR,
                    "Device unresponsive: " + d.device_name + " at " + d.address[0] + ":" + str(d.address[1])
                )

                # Vocalise an alert -
                self.tts.play("Alert, " + d.device_name + " is not responding")

                # Raise an incident -
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

        if packet.msg_code == dosa.Messages.LOG:
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
            elif log_level == dosa.LogLevel.ERROR:
                msg = "Warning, " + packet.device_name + " error. " + log_message + "."

            if msg:
                # Raise an incident for this log message
                self.alert(
                    packet.device_name,
                    packet.device_name + " critical",
                    description=log_message,
                    category=dosa.AlertCategory.NETWORK,
                    level=dosa.LogLevel.as_string(log_level)
                )

        elif packet.msg_code == dosa.Messages.SEC:
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
            msg = "Network flush initiated by " + packet.device_name
            self.log(packet)
            self.devices.clear()
            self.last_ping = 0

        elif packet.msg_code == dosa.Messages.TRIGGER:
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
            self.comms.send_ack(packet.msg_id_bytes(), packet.addr)
            play = packet.payload[27:packet.payload_size].decode("utf-8")
            self.log(packet, " | PLAY | " + play)
            self.run_play(play)

        elif packet.msg_code == dosa.Messages.PONG:
            # Ignore in logs, but register/update device
            match = False
            for d in self.devices:
                if d.address == device.address:
                    match = True
                    d.pong()
                    if d.reported_unresponsive:
                        # device recovery
                        d.reported_unresponsive = False
                        self.comms.net_log(dosa.LogLevel.WARNING, "Device recovery: " + d.device_name)
                        if self.report_recovery:
                            msg = "Notice, " + d.device_name + " is back online"
                    break

            if not match:
                device.device_type = packet.payload[self.comms.BASE_PAYLOAD_SIZE]
                device.device_state = packet.payload[self.comms.BASE_PAYLOAD_SIZE + 1]
                self.devices.append(device)
                print("Found device: " + device.device_name)

        elif packet.msg_code == dosa.Messages.PING or packet.msg_code == dosa.Messages.ACK:
            # Don't log pings or acks
            pass

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

    def alert(self, device, msg, category, level, description=None, tags=None):
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

        cat = tags["category"]  # keep a copy of the pure category before we colourise it
        if cat not in self.settings["alerts"]:
            return

        tags = self.colourise_tags(tags)
        payload = {"message": msg, "description": description, "tags": tags, "status": "trigger"}

        for endpoint in self.settings["alerts"][cat]:
            r = requests.post(endpoint, json=payload, headers={"Content-Type": "application/json"})

            if r.status_code == 200:
                print(category + " alert dispatched to " + endpoint)
                self.comms.net_log(
                    dosa.LogLevel.WARNING,
                    category + " alert dispatched to " + endpoint
                )
            else:
                # Don't set to ERROR or above, else you'll get infinite recursion
                self.comms.net_log(
                    dosa.LogLevel.ERROR,
                    "SecBot failed to page alert for device " + device + ", response code: " + str(r.status_code)
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
    def colourise_tags(tags):
        if tags["category"] == dosa.AlertCategory.SECURITY:
            tags["category"] = {"color": "#FCB900", "value": dosa.AlertCategory.SECURITY}

        if tags["level"] == dosa.LogLevel.as_string(dosa.LogLevel.ERROR) or \
                tags["level"] == dosa.SecurityLevel.as_string(dosa.SecurityLevel.TAMPER) or \
                tags["level"] == dosa.SecurityLevel.as_string(dosa.SecurityLevel.ALERT):
            tags["level"] = {"color": "#FCB900", "value": tags["level"]}

        elif tags["level"] == dosa.LogLevel.as_string(dosa.LogLevel.CRITICAL) or \
                tags["level"] == dosa.SecurityLevel.as_string(dosa.SecurityLevel.BREACH) or \
                tags["level"] == dosa.SecurityLevel.as_string(dosa.SecurityLevel.PANIC):
            tags["level"] = {"color": "#EB144C", "value": tags["level"]}

        return tags

    @staticmethod
    def get_current_time():
        return int(time.time())
