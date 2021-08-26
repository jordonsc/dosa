#pragma once

#include "dosa_sensor.h"

namespace dosa {

class DevicePool
{
   public:
    DevicePool(DevicePool const&) = delete;
    void operator=(DevicePool const&) = delete;

    static DevicePool& getInstance()
    {
        static DevicePool instance;
        return instance;
    }

    Sensor& getDevice(unsigned short index)
    {
        return devices[index];
    }

    bool exists(unsigned short index)
    {
        return devices[index].operator bool();
    }

    void add(BLEDevice const& device)
    {
        // Check if we already know about this device, if we do - the peripheral gave up on us :)
        for (auto& d : devices) {
            if (d && (d.getAddress() == device.address())) {
                d.disconnect();
                if (d.connect(device)) {
                    d.discover();
                }
                return;
            }
        }

        // Not a known device, add to a free slot
        for (auto& d : devices) {
            if (!d) {
                if (d.connect(device)) {
                    d.discover();
                }
                return;
            }
        }

        // Overflow - will only reach here if there are no free slots to add
        auto& lights = dosa::Lights::getInstance();

        serial.writeln(
            "Unable to register sensor (" + device.address() + "): hit device limit",
            dosa::LogLevel::WARNING);

        lights.off();
        errorSignal();
        delay(500);
    }

    /**
     * Poll all devices if their polling frequency is up.
     */
    unsigned int process()
    {
        unsigned int connected = 0;

        for (auto& d : devices) {
            if (!d) {
                continue;
            }

            if (d.shouldPoll(poll_freq) && d.poll()) {
                serial.writeln("Device " + d.getAddress() + " new state: " + d.getState());

                // Polling may have invalidated the connection. If d is still returning true, then the connection lives
                if (d) {
                    ++connected;
                }
            } else {
                // Testing d.connected() will require a poll - which we don't want - so just assume connection is still
                // good
                ++connected;
            }
        }

        return connected;
    }

   private:
    Sensor devices[max_devices];
    dosa::SerialComms& serial;

    DevicePool() : serial(dosa::SerialComms::getInstance()) {}
};

}  // namespace dosa
