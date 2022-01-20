/**
 * WiFi library for NINA chips.
 */

#pragma once

#include <WiFiNINA.h>

#include "const.h"
#include "loggable.h"

namespace dosa {

class Wifi : public Loggable
{
   public:
    explicit Wifi(SerialComms* s = nullptr) : Loggable(s) {}

    bool connect(String const& ssid, String const& password, uint8_t attempts = 10)
    {
        if (isConnected()) {
            disconnect();
        }

        WiFi.setTimeout(30 * 1000);
        uint8_t attempt = 0;

        do {
            if (attempt > 0) {
                WiFi.end();
                log("Retry " + String(attempt + 1) + "/" + String(attempts) + ".. ");
            }

            // Connect to WPA/WPA2 network:
            status = WiFi.begin(ssid.c_str(), password.c_str());

            if (status != WL_CONNECTED) {
                switch (status) {
                    case WL_CONNECT_FAILED:
                        logln("Connection failed");
                        break;
                    case WL_AP_FAILED:
                        logln("Connection failed: AP error");
                        break;
                    case WL_CONNECTION_LOST:
                        logln("Connection lost");
                        break;
                    case WL_DISCONNECTED:
                        logln("Disconnected");
                        break;
                    case WL_FAILURE:
                        logln("Wifi failure");
                        break;
                    default:
                        logln("Unknown connection error: " + String(status));
                }

                ++attempt;
            } else {
                logln(
                    "Connected to " + ssid + "; local IP: " + comms::ipToString(WiFi.localIP()) + " netmask " +
                    comms::ipToString(WiFi.subnetMask()));
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

   protected:
    int status = WL_DISCONNECTED;
    WiFiUDP udp;
};

}  // namespace dosa
