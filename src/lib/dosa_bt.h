/**
 * Bluetooth Comms.
 */

#pragma once

#include <ArduinoBLE.h>

#include "dosa_error.h"
#include "dosa_lights.h"
#include "dosa_serial.h"

namespace dosa {

class Bluetooth
{
   public:
    Bluetooth(Bluetooth const&) = delete;
    void operator=(Bluetooth const&) = delete;

    static Bluetooth& getInstance()
    {
        static Bluetooth instance;
        return instance;
    }

    void setAppearance(uint16_t value) const
    {
        if (!enabled) {
            auto& serial = dosa::SerialComms::getInstance();
            serial.writeln("BT disabled, cannot alter appearance", LogLevel::ERROR);
            return;
        }

        BLE.setAppearance(value);
    }

    /**
     * Set the connection interval in multiples of 1.25ms.
     * milliseconds = value * 1.25
     *
     * Min cannot be lower than 7.5ms (6)
     * Max cannot be higher than 4s (3200)
     */
    void setConnectionInterval(uint16_t min, uint16_t max) const
    {
        if (!enabled) {
            auto& serial = dosa::SerialComms::getInstance();
            serial.writeln("BT disabled, cannot alter connection interval", LogLevel::ERROR);
            return;
        }

        if (min < 6 || max < 6 || min > 3200 || max > 3200 || max < min) {
            auto& serial = dosa::SerialComms::getInstance();
            serial.writeln("Connection interval values are invalid", LogLevel::ERROR);
            return;
        }

        BLE.setConnectionInterval(min, max);
    }

    String localAddress()
    {
        return BLE.address();
    }

    bool setEnabled(bool value)
    {
        auto& serial = dosa::SerialComms::getInstance();
        if (value) {
            serial.writeln("Enabling BLE", LogLevel::DEBUG);
            enabled = (bool)BLE.begin();
        } else {
            serial.writeln("Disabling BLE", LogLevel::DEBUG);
            BLE.end();
            enabled = false;
        }

        return enabled;
    }

    [[nodiscard]] bool isEnabled() const
    {
        return enabled;
    }

    /**
     * Set the BT broadcast name.
     *
     * Returns false on failure.
     */
    bool setName(String const& name)
    {
        auto& serial = dosa::SerialComms::getInstance();
        serial.writeln("Local name: " + name);

        // IMPORTANT: updating or invalidating the reference passed will update (or break) the BLE local name, so it's
        // important to store in a local variable.
        localName = name;
        return BLE.setLocalName(localName.c_str());
    }

    /**
     * Toggle the BT advertising state.
     *
     * This will NOT turn the BT on or off, BT can be enabled but not advertising.
     *
     * Returns false on failure.
     */
    bool setAdvertise(bool advertise) const
    {
        if (enabled) {
            if (advertise) {
                return (bool)BLE.advertise();
            } else {
                BLE.stopAdvertise();
                return true;
            }
        } else {
            auto& serial = dosa::SerialComms::getInstance();
            serial.writeln("BT disabled, cannot alter advertising state", LogLevel::ERROR);
            return false;
        }
    }

    [[nodiscard]] BLEDevice scanForService(String const& service) const
    {
        auto& serial = dosa::SerialComms::getInstance();
        if (enabled) {
            serial.writeln("Scanning for service " + service, LogLevel::DEBUG);
            BLE.scanForUuid(service);
            return BLE.available();
        } else {
            serial.writeln("Attempting to scan when disabled", LogLevel::ERROR);
            return BLEDevice();
        }
    }

   private:
    Bluetooth()
    {
        BLE.setDeviceName("DOSA");
        localName = "DOSA";
    }

    bool enabled = false;
    String localName;
};

}  // namespace dosa
