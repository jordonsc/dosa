import dosa
import struct
import time
import os
from dosa.tts import Tts


class SecBot:
    """
    Monitors for security alerts and errors. Vocalises them though TTS.
    """

    ping_interval = 10  # Time in seconds between pings
    heartbeat_interval = 15  # Time in seconds between heartbeats
    warning_age = 35  # Time in seconds before declaring a device as tentative
    error_age = 65  # Time in seconds before declaring a device offline

    statsd_server = {"server": os.getenv("DOSA_STATS_SERVER", "127.0.0.1"),
                     "port": int(os.getenv("DOSA_STATS_PORT", "8125"))}
    log_server = {"server": os.getenv("DOSA_LOG_SERVER", "127.0.0.1"),
                  "port": int(os.environ.get("DOSA_LOG_PORT", "10518"))}

    def __init__(self, comms=None, voice="Emma", engine="neural"):
        if comms is None:
            comms = dosa.Comms()

        self.last_msg_id = 0
        self.comms = comms
        self.tts = Tts(voice=voice, engine=engine)
        self.last_ping = 0
        self.last_heartbeat = 0
        self.devices = []

    def run(self):
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
        ct = self.get_current_time()

        # Send a ping if we're stale
        if ct - self.last_ping > self.ping_interval:
            self.comms.send(self.comms.build_payload(dosa.Messages.PING))
            self.last_ping = ct

    def check_for_packets(self):
        packet = self.comms.receive(timeout=0.1)

        if packet is None or packet.msg_id == self.last_msg_id:
            return

        self.last_msg_id = packet.msg_id
        msg = ""

        if packet.msg_code == dosa.Messages.LOG:
            log_level = struct.unpack("<B", packet.payload[27:28])[0]
            log_message = packet.payload[28:packet.payload_size].decode("utf-8")
            aux = " | " + dosa.LogLevel.as_string(log_level) + " | " + log_message
            self.log(packet, aux)

            if log_level == dosa.LogLevel.SECURITY:
                msg = "Security alert, " + packet.device_name + ". " + log_message + "."
            elif log_level == dosa.LogLevel.CRITICAL:
                msg = "Warning, " + packet.device_name + " critical. " + log_message + "."
            elif log_level == dosa.LogLevel.ERROR:
                msg = "Warning, " + packet.device_name + " error. " + log_message + "."

        elif packet.msg_code == dosa.Messages.FLUSH:
            msg = "Network flush initiated by " + packet.device_name
            self.log(packet)

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

        elif packet.msg_code == dosa.Messages.PING or packet.msg_code == dosa.Messages.PONG:
            # Ignore ping/pong messages in logs
            pass

        else:
            self.log(packet)

        if msg:
            print(msg)
            self.tts.play(msg, wait=False)

    def log(self, msg, aux=""):
        t = time.strftime("%H:%M:%S", time.localtime())
        payload = t + " [" + str(msg.msg_id).rjust(5, ' ') + "] " + msg.addr[0] + ":" + str(
            msg.addr[1]) + " (" + msg.device_name + "): " + msg.msg_code.decode("utf-8").upper() + aux
        self.comms.send(payload.encode(), (self.log_server["server"], self.log_server["port"]))

    @staticmethod
    def get_current_time():
        return int(time.time())
