import dosa
import struct
from dosa.tts import Tts


class SecBot:
    """
    Monitors for security alerts and errors. Vocalises them though TTS.
    """

    def __init__(self, comms=None, voice="Emma", engine="neural"):
        if comms is None:
            comms = dosa.Comms()

        self.last_msg_id = 0
        self.comms = comms
        self.tts = Tts(voice=voice, engine=engine)

    def run(self):
        self.tts.play("DOSA Security Bot online")

        while True:
            packet = self.comms.receive(timeout=None)
            msg = ""

            if packet.msg_id == self.last_msg_id:
                continue

            self.last_msg_id = packet.msg_id

            if packet.msg_code == dosa.Messages.LOG:
                log_level = struct.unpack("<B", packet.payload[27:28])[0]
                log_message = packet.payload[28:packet.payload_size].decode("utf-8")
                if log_level == dosa.LogLevel.SECURITY:
                    msg = "Security alert, " + packet.device_name + ". " + log_message + "."
                elif log_level == dosa.LogLevel.CRITICAL:
                    msg = packet.device_name + " critical! " + log_message + "."
                elif log_level == dosa.LogLevel.CRITICAL:
                    msg = packet.device_name + " error. " + log_message + "."

            elif packet.msg_code == dosa.Messages.FLUSH:
                msg = "Network flush initiated by " + packet.device_name

            if msg:
                print(msg)
                self.tts.play(msg, wait=True)
