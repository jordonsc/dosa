#pragma once

#include <ArduinoBLE.h>

#include "dosa_const.h"
#include "dosa_serial.h"

namespace dosa {

class Sensor
{
   public:
    Sensor() : serial(dosa::SerialComms::getInstance()) {}

    [[nodiscard]] unsigned short getState() const
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
        serial.writeln("Polling device " + device.address() + "..", dosa::LogLevel::TRACE);
        last_poll = millis();
        device.poll();

        if (sensor && sensor.valueUpdated()) {
            sensor.readValue(state);
            return true;
        }

        return false;
    }

    bool connect(BLEDevice const& d)
    {
        device = d;
        serial.write("Connecting to " + device.address() + "..");

        if (device.connect()) {
            serial.writeln(" OK");
            return true;
        } else {
            serial.writeln(" FAILED");
            disconnect();
            return false;
        }
    }

    void disconnect()
    {
        serial.writeln("Disconnecting from " + device.address());
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

        serial.write("Device " + device.address() + " discovery in progress..");
        if (device.discoverAttributes()) {
            sensor = device.characteristic(sensor_char_id);
            if (!sensor) {
                serial.writeln(" ERROR - no sensor discovered");
                disconnect();
                return false;
            } else {
                serial.writeln(" OK");
                serial.write("Subscribing..");
                if (sensor.subscribe()) {
                    serial.writeln(" OK");
                    return true;
                } else {
                    serial.writeln(" ERROR");
                    disconnect();
                    return false;
                }
            }
        } else {
            serial.writeln(" FAILED");
            disconnect();
            return false;
        }
    }

   protected:
    BLEDevice device;
    BLECharacteristic sensor;

    unsigned short state = 0;
    unsigned long last_poll = 0;
    dosa::SerialComms& serial;
};

}  // namespace dosa
