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
            serial.writeln("Enabling BLE..", LogLevel::DEBUG);
            enabled = BLE.begin() == 1;

            // deviceName = "DOSA" + localAddress().substring(15);
            // BLE.setDeviceName(deviceName.c_str());

            // BLE.setTimeout(10000);
        } else {
            serial.writeln("Disabling BLE..", LogLevel::DEBUG);
            BLE.end();
            enabled = false;
        }

        return enabled;
    }

    [[nodiscard]] bool isEnabled() const
    {
        return enabled;
    }

    [[nodiscard]] bool isScanning() const
    {
        return scanning;
    }

    [[nodiscard]] bool isAdvertising() const
    {
        return advertising;
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
     * Toggle the BT advertising & connectability state.
     *
     * This will NOT turn the BT on or off, BT can be enabled but not advertising.
     *
     * Returns false on failure.
     */
    bool setAdvertise(bool advertise)
    {
        if (advertise) {
            BLE.setConnectable(true);
            advertising = BLE.advertise() == 1;
        } else {
            BLE.setConnectable(false);
            BLE.stopAdvertise();
            advertising = false;
        }

        return advertising;
    }

    void scanForService(String const& service)
    {
        auto& serial = dosa::SerialComms::getInstance();
        serial.writeln("Scanning for service " + service, LogLevel::DEBUG);
        BLE.scanForUuid(service);
        scanning = true;
    }

    void stopScan()
    {
        BLE.stopScan();
        scanning = false;
    }

   private:
    Bluetooth()
    {
        localName = "DOSA";
        deviceName = "DOSA";
        BLE.setDeviceName(deviceName.c_str());
    }

    bool enabled = false;
    bool scanning = false;
    bool advertising = false;
    String localName;
    String deviceName;
};

}  // namespace dosa
