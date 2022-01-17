/**
 * WiFi library for NINA chips.
 */

#pragma once

#include <WiFiNINA.h>

#include "loggable.h"

namespace dosa {

class Wifi : public Loggable
{
   public:
    explicit Wifi(SerialComms* s = nullptr) : Loggable(s) {}

    bool connect(String const& ssid, String const& password, uint8_t attempts = 30)
    {
        if (isConnected()) {
            disconnect();
        }

        WiFi.setTimeout(30 * 1000);
        uint8_t attempt = 0;

        do {
            if (attempt > 0) {
                WiFi.end();
                log("Retry " + String(attempt + 1) + "/" + String(attempts) + "..");
            }

            // Connect to WPA/WPA2 network:
            status = WiFi.begin(ssid.c_str(), password.c_str());

            if (status != WL_CONNECTED) {
                logln("Connection failed (" + String(status) + ")");
                ++attempt;
            } else {
                logln(
                    "Connected to " + ssid + "; local IP: " + ipToString(WiFi.localIP()) + " netmask " +
                    ipToString(WiFi.subnetMask()));
                return true;
            }
        } while (attempt < attempts);

        return false;
    }

    void disconnect()
    {
        WiFi.disconnect();
        status = WL_DISCONNECTED;
        logln("Wifi disconnected");
    }

    [[nodiscard]] WiFiUDP& getUdp()
    {
        return udp;
    }

    [[nodiscard]] bool isConnected() const
    {
        return status == WL_CONNECTED;
    }

    [[nodiscard]] int getStatus() const
    {
        return status;
    }

    [[nodiscard]] static String ipToString(IPAddress const& ip)
    {
        return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
    }

   protected:
    int status = WL_DISCONNECTED;
    WiFiUDP udp;
};

}  // namespace dosa
