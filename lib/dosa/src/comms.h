#pragma once

#include <messages.h>

namespace dosa {

class Comms : public Loggable
{
   public:
    explicit Comms(Wifi& wifi, SerialComms* s = nullptr) : Loggable(s), wifi(wifi) {}

    bool dispatch(IPAddress const& ip, unsigned long port, messages::Payload const& payload, bool wait_for_ack = false) {
        static int retries = 0;

        if (!wifi.isConnected()) {
            return false;
        }

        auto& udp = wifi.getUdp();

        if (udp.beginPacket(ip, port) != 1) {
            logln("ERROR: UDP begin failed", dosa::LogLevel::ERROR);
            return false;
        }

        udp.write(payload.getPayload(), payload.getPayloadSize());

        if (udp.endPacket() != 1) {
            logln("ERROR: UDP end failed", dosa::LogLevel::ERROR);
            return false;
        }

        // Wait for ack
        auto start = millis();
        char buffer[255];
        do {
            if (udp.parsePacket()) {
                auto data_size = min(udp.available(), 255);
                udp.read(buffer, data_size);
                buffer[data_size] = 0x0;
                auto incoming = String(buffer);
                if (incoming == "ack") {
                    serial.writeln(
                        "Trigger acknowledged by " + dosa::Wifi::ipToString(udp.remoteIP()),
                        dosa::LogLevel::INFO);
                    return;
                } else {
                    serial.writeln(
                        "Unexpected reply '" + incoming + "' from " + dosa::Wifi::ipToString(udp.remoteIP()),
                        dosa::LogLevel::WARNING);
                }
                break;
            }
        } while (millis() - start < 250);

        // Didn't get an ack in a timely manner, retry the notification
        if (retries < MAX_ACK_RETRIES) {
            dispatchNotification(retries + 1);
        } else {
            serial.writeln("WARNING: trigger has gone unacknowledged", dosa::LogLevel::WARNING);
        }
    }

   protected:
    Wifi& wifi;
};

}  // namespace dosa