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
