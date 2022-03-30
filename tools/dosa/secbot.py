import dosa
import struct
import time
from dosa.tts import Tts


class SecBot:
    """
    Monitors for security alerts and errors. Vocalises them though TTS.
    """

    ping_interval = 10  # Time in seconds between pings
    heartbeat_interval = 15  # Time in seconds between heartbeats
    warning_age = 35  # Time in seconds before declaring a device as tentative
    error_age = 65  # Time in seconds before declaring a device offline
    # statsd_server = ("127.0.0.1", 8125)  # Statsd server (should be local to the SecBot agent)
    statsd_server = ("192.168.0.8", 8125)

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
        self.tts.play("DOSA Security Bot online")

        while True:
            self.do_heartbeat()
            self.check_devices()
            self.check_for_packets()

    def do_heartbeat(self):
        ct = self.get_current_time()

        # Send a heartbeat if we're stale
        if ct - self.last_heartbeat > self.heartbeat_interval:
            self.comms.send("dosa.secbot.heartbeat:1|c".encode(), self.statsd_server)
            self.last_heartbeat = ct

    def check_devices(self):
        pass
        # ct = self.get_current_time()
        #
        # # Send a ping if we're stale
        # if ct - self.last_ping > self.ping_interval:
        #     self.comms.send(self.comms.build_payload(dosa.Messages.PING))
        #     self.last_ping = ct

    def check_for_packets(self):
        packet = self.comms.receive(timeout=0.1)

        if packet is None or packet.msg_id == self.last_msg_id:
            return

        self.last_msg_id = packet.msg_id
        msg = ""

        if packet.msg_code == dosa.Messages.LOG:
            log_level = struct.unpack("<B", packet.payload[27:28])[0]
            log_message = packet.payload[28:packet.payload_size].decode("utf-8")
            if log_level == dosa.LogLevel.SECURITY:
                msg = "Security alert, " + packet.device_name + ". " + log_message + "."
            elif log_level == dosa.LogLevel.CRITICAL:
                msg = "Warning, " + packet.device_name + " critical. " + log_message + "."
            elif log_level == dosa.LogLevel.ERROR:
                msg = "Warning, " + packet.device_name + " error. " + log_message + "."

        elif packet.msg_code == dosa.Messages.FLUSH:
            msg = "Network flush initiated by " + packet.device_name

        if msg:
            print(msg)
            self.tts.play(msg, wait=False)

    @staticmethod
    def get_current_time():
        return int(time.time())
