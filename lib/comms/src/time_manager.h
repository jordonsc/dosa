#pragma once

#include <Arduino.h>

#define NTP_PACKET_SIZE 76
#define DOSA_NTP_WAIT 1500

namespace dosa {
namespace comms {

class TimeManager : public Loggable
{
   public:
    explicit TimeManager(Wifi& wifi, SerialComms* s = nullptr) : Loggable(s), wifi(wifi) {}

    [[nodiscard]] time_t getNtpTime(comms::Node const& ntp_server, int8_t timezone = 0)
    {
        sendNtpPacket(ntp_server);
        auto& udp = wifi.getUdp();

        uint32_t start_time = millis();
        while (millis() - start_time < DOSA_NTP_WAIT) {
            if (!udp.parsePacket()) {
                delay(10);
                continue;
            }

            auto data_size = uint32_t(udp.available());
            if (data_size == NTP_PACKET_SIZE) {
                logln("Received NTP packet", LogLevel::DEBUG);

                byte packet[NTP_PACKET_SIZE] = {0};
                udp.read(packet, NTP_PACKET_SIZE);

                unsigned long timestamp;
                timestamp = (unsigned long)packet[40] << 24;
                timestamp |= (unsigned long)packet[41] << 16;
                timestamp |= (unsigned long)packet[42] << 8;
                timestamp |= (unsigned long)packet[43];
                return timestamp - 2208988800UL + timezone * 3600;
            } else {
                logln("NTP: received incorrect packet size (" + String(udp.available()) + ")", LogLevel::DEBUG);
                udp.flush();
            }
        }

        logln("Did not receive NTP reply", LogLevel::ERROR);
        return 0;
    }

   protected:
    Wifi& wifi;

    void sendNtpPacket(Node const& ntp_server)
    {
        byte payload[NTP_PACKET_SIZE] = {0};

        // Initialize values needed to form NTP request
        payload[0] = 0b11100011;  // LI, Version, Mode
        payload[1] = 0;           // Stratum, or type of clock
        payload[2] = 6;           // Polling Interval
        payload[3] = 0xEC;        // Peer Clock Precision
        // 8 bytes of zero for Root Delay & Root Dispersion
        payload[12] = 49;
        payload[13] = 0x4E;
        payload[14] = 49;
        payload[15] = 52;

        sendRawPacket(ntp_server, payload, NTP_PACKET_SIZE);
    }

    bool sendRawPacket(IPAddress const& ip, uint16_t port, uint8_t const* payload, uint32_t size)
    {
        auto& udp = wifi.getUdp();

        if (udp.beginPacket(ip, port) != 1) {
            logln("ERROR: UDP begin failed", dosa::LogLevel::ERROR);
            return false;
        }

#ifdef ARDUINO_ARCH_SAMD
        udp.write((char*)payload, size);
#endif
#ifdef ARDUINO_ESP32_DEV
        udp.write(payload, size);
#endif

        if (udp.endPacket() != 1) {
            logln("ERROR: UDP end failed", dosa::LogLevel::ERROR);
            return false;
        }
    }

    bool sendRawPacket(comms::Node const& target, uint8_t const* payload, uint32_t size)
    {
        return sendRawPacket(target.ip, target.port, payload, size);
    }
};

}  // namespace comms
}  // namespace dosa