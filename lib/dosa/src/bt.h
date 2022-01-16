/**
 * Bluetooth library for central and peripheral devices.
 */

#pragma once

#include <ArduinoBLE.h>

#include "loggable.h"

namespace dosa {

class Bluetooth : public Loggable
{
   public:
    explicit Bluetooth(SerialComms* s = nullptr) : Loggable(s)
    {
        localName = "DOSA";
        deviceName = "DOSA";
        BLE.setDeviceName(deviceName.c_str());
    }

    void setAppearance(uint16_t value) const
    {
        if (!enabled) {
            logln("BT disabled, cannot alter appearance", LogLevel::ERROR);
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
            logln("BT disabled, cannot alter connection interval", LogLevel::ERROR);
            return;
        }

        BLE.setConnectionInterval(min, max);
    }

    [[nodiscard]] String localAddress() const
    {
        return BLE.address();
    }

    bool setEnabled(bool value)
    {
        if (value) {
            logln("Enabling BLE..", LogLevel::DEBUG);
            enabled = BLE.begin() == 1;
        } else {
            logln("Disabling BLE..", LogLevel::DEBUG);
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
    bool setLocalName(String const& name)
    {
        logln("Local name: " + name);

        // IMPORTANT: updating or invalidating the reference passed will update (or break) the BLE local name, so it's
        // important to store in a local variable.
        localName = name;
        return BLE.setLocalName(localName.c_str());
    }

    /**
     * Set the BT device name.
     */
    void setDeviceName(String const& name)
    {
        logln("Device name: " + name);

        // IMPORTANT: updating or invalidating the reference passed will update (or break) the BLE device name, so it's
        // important to store in a local variable.
        deviceName = name;
        BLE.setDeviceName(deviceName.c_str());
    }

    bool setService(BLEService& svc)
    {
        if (!BLE.setAdvertisedService(svc)) {
            return false;
        }

        BLE.addService(svc);
        return true;
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
        logln("Scanning for service " + service, LogLevel::DEBUG);
        BLE.scanForUuid(service);
        scanning = true;
    }

    void stopScan()
    {
        BLE.stopScan();
        scanning = false;
    }

   private:
    bool enabled = false;
    bool scanning = false;
    bool advertising = false;
    String localName;
    String deviceName;
};

}  // namespace dosa
