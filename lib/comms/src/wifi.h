/**
 * WiFi library for NINA chips.
 */

#pragma once

#include <Arduino.h>

#ifdef ARDUINO_ARCH_SAMD
#include <WiFiNINA.h>
#else
#include <WiFi.h>
#endif

#include "const.h"
#include "loggable.h"

#define DOSA_WIFI_CONNECT_TIMEOUT 10000

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

        reset();
        ctrl.setHostname(hostname.c_str());

        return connectSequence(ssid, password, attempts);
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
        return wifi_online ? getControllerStatus() : WL_DISCONNECTED;
    }

    [[nodiscard]] int getControllerStatus()
    {
#ifdef ARDUINO_ARCH_SAMD
        return ctrl.status();
#else
        return WiFiClass::status();
#endif
    }

    /**
     * Do a hard reset on the wifi driver, required after switching back from Bluetooth.
     *
     * This only applies to NINA chips.
     */
    void reset()
    {
#ifdef ARDUINO_ARCH_SAMD
        WiFiDrv::wifiDriverDeinit();
        WiFiDrv::wifiDriverInit();
#endif
    }

    String const& getHostname() const
    {
        return hostname;
    }

    void setHostname(String value)
    {
        hostname = std::move(value);
    }

   protected:
    bool wifi_online = false;
    WiFiClass& ctrl;
    WiFiUDP udp;
    String hostname = "dosa";

    void logStatusCode(int status)
    {
        switch (status) {
            case WL_CONNECTED:
                logln("Connected");
                break;
            case WL_CONNECT_FAILED:
                logln("Connection failed");
                break;
#ifdef ARDUINO_ARCH_SAMD
            case WL_AP_FAILED:
                logln("Connection failed: AP error");
                break;
            case WL_FAILURE:
                logln("Wifi failure");
                break;
#endif
            case WL_CONNECTION_LOST:
                logln("Connection lost");
                break;
            case WL_DISCONNECTED:
                logln("Disconnected");
                break;
            case WL_NO_SSID_AVAIL:
                logln("No SSID available");
                break;
            default:
                logln("Unknown connection error: " + String(status));
        }
    }

    void logLocalIp()
    {
        logln("Local IP: " + comms::ipToString(ctrl.localIP()) + " netmask " + comms::ipToString(ctrl.subnetMask()));
    }

    bool connectSequence(String const& ssid, String const& password, uint8_t attempts)
    {
#ifdef ARDUINO_ARCH_SAMD
        ctrl.setTimeout(DOSA_WIFI_CONNECT_TIMEOUT);
#endif
        uint8_t attempt = 0;

        do {
            if (attempt > 0) {
#ifdef ARDUINO_ARCH_SAMD
                ctrl.end();
#else
                ctrl.disconnect(true, true);
#endif
                log("Retry " + String(attempt + 1) + "/" + String(attempts) + ".. ");
            }

            // Connect to WPA/WPA2 network:
            auto start_time = millis();
            int status = ctrl.begin(ssid.c_str(), password.c_str());

            while (status != WL_CONNECTED) {
                if (millis() - start_time > DOSA_WIFI_CONNECT_TIMEOUT) {
                    break;
                } else {
                    delay(100);
                    status = getControllerStatus();
                }
            }

            if (status != WL_CONNECTED) {
                wifi_online = false;
                logStatusCode(status);
                ++attempt;
            } else {
                wifi_online = true;
                logln("Wifi connected");
                logLocalIp();

                return true;
            }
        } while (attempt < attempts);

        ctrl.disconnect();
        return false;
    }
};

}  // namespace dosa
