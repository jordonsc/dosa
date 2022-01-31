/**
 * WiFi library for NINA chips.
 */

#pragma once

#include <WiFiNINA.h>

#include "const.h"
#include "loggable.h"

#define DOSA_WIFI_CONNECT_TIMEOUT 15000

namespace dosa {

class Wifi : public Loggable
{
   public:
    explicit Wifi(SerialComms* s = nullptr) : Loggable(s), ctrl(WiFi) {}

    bool connect(String const& ssid, String const& password, uint8_t attempts = 10)
    {
        if (isConnected()) {
            logln("Wifi online, disconnecting..");
            disconnect();
        }

        ctrl.setHostname("DOSA");
        ctrl.setTimeout(DOSA_WIFI_CONNECT_TIMEOUT);
        uint8_t attempt = 0;

        do {
            if (attempt > 0) {
                ctrl.end();
                log("Retry " + String(attempt + 1) + "/" + String(attempts) + ".. ");
            }

            // Connect to WPA/WPA2 network:
            int status = ctrl.begin(ssid.c_str(), password.c_str());

            if (status != WL_CONNECTED) {
                wifi_online = false;

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
                    case WL_NO_SSID_AVAIL:
                        logln("No SSID available");
                        break;
                    default:
                        logln("Unknown connection error: " + String(status));
                }

                ++attempt;
            } else {
                wifi_online = true;
                logln("Wifi connected");
                logln(
                    "Local IP: " + comms::ipToString(ctrl.localIP()) + " netmask " +
                    comms::ipToString(ctrl.subnetMask()));

                return true;
            }
        } while (attempt < attempts);

        ctrl.end();
        return false;
    }

    void disconnect()
    {
        ctrl.disconnect();
        wifi_online = false;
        logln("Wifi disconnected");
    }

    [[nodiscard]] WiFiUDP& getUdp()
    {
        return udp;
    }

    [[nodiscard]] bool isConnected()
    {
        return wifi_online && (getStatus() == WL_CONNECTED);
    }

    [[nodiscard]] int getStatus()
    {
        return wifi_online ? ctrl.status() : WL_DISCONNECTED;
    }

   protected:
    bool wifi_online = false;
    WiFiClass& ctrl;
    WiFiUDP udp;
};

}  // namespace dosa
