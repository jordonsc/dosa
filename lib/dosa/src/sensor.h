/**
 * Represents a client Sensor.
 */

#pragma once

#include <Arduino.h>

#include "const.h"
#include "loggable.h"

namespace dosa {

class Sensor : public Loggable
{
   public:
    explicit Sensor(dosa::SerialComms* s = nullptr) : Loggable(s) {}

    [[nodiscard]] byte getState() const
    {
        return state;
    }

    [[nodiscard]] bool shouldPoll(unsigned long freq) const
    {
        return millis() - last_poll > freq;
    }

    [[nodiscard]] bool isConnected() const
    {
        return device.connected();
    }

    [[nodiscard]] bool hasDiscovered() const
    {
        return sensor;
    }

    [[nodiscard]] String getAddress() const
    {
        return device.address();
    }

    explicit operator bool() const
    {
        return device;
    }

    bool poll()
    {
        serial->writeln("Polling device " + device.address() + "..", dosa::LogLevel::TRACE);
        last_poll = millis();
        device.poll();

        if (sensor) {
            byte new_state;
            sensor.readValue(new_state);
            if (new_state == 0) {
                serial->writeln("Read error on device: " + device.address(), dosa::LogLevel::WARNING);
                disconnect();
            } else if (new_state != state) {
                state = new_state;
                return true;
            }
        } else {
            // Not sure why this would happen?
            serial->writeln("Sensor failure on device: " + device.address(), dosa::LogLevel::WARNING);
            disconnect();
        }

        return false;
    }

    bool connect(BLEDevice const& d)
    {
        device = d;
        log("Connecting to " + device.address() + "..");

        if (device.connect()) {
            logln(" OK");
            return true;
        } else {
            logln(" FAILED");
            disconnect();
            return false;
        }
    }

    void disconnect()
    {
        logln("Disconnecting from " + device.address());
        device = BLEDevice();
        sensor = BLECharacteristic();
        last_poll = 0;
        state = 0;
    }

    bool discover()
    {
        if (!isConnected()) {
            disconnect();
            return false;
        }

        log("Device " + device.address() + " discovery in progress..");
        if (device.discoverAttributes()) {
            sensor = device.characteristic(dosa::bt::char_pir);
            if (!sensor) {
                logln(" ERROR - no sensor discovered");
                disconnect();
                return false;
            } else {
                logln(" OK");
                return true;
                log("Subscribing..");
                if (sensor.subscribe()) {
                    logln(" OK");
                    return true;
                } else {
                    logln(" ERROR");
                    disconnect();
                    return false;
                }
            }
        } else {
            logln(" FAILED");
            disconnect();
            return false;
        }
    }

   protected:
    BLEDevice device;
    BLECharacteristic sensor;

    byte state = 0;
    unsigned long last_poll = 0;
};

}  // namespace dosa
