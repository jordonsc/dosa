#pragma once

#include <dosa.h>

#include "door_container.h"

#define NO_DEVICE_BLINK_INTERVAL 500

namespace dosa::door {

class DoorApp : public dosa::App
{
   public:
    using dosa::App::App;

    void init() override
    {
        App::init();
        container.getDoorLights().ready();
    }

    void loop() override
    {
        auto& serial = container.getSerial();
        auto& bt = container.getBluetooth();
        auto& pool = container.getDevicePool();
        auto& door_lights = container.getDoorLights();

        // Peripheral scan timer
        static unsigned long last_scan = 0;

        // When no devices are ready, we'll use this timer to blink the ready LED
        static unsigned long blink_timer = 0;
        static bool blink_state = true;

        // Check the door switch
        auto& door_switch = container.getDoorSwitch();
        if (door_switch.process() && door_switch.getState()) {
            serial.writeln("Door switch pressed");
            door_lights.activity();
            container.getDoorWinch().trigger();
            door_lights.ready();
            blink_state = true;
        }

        // Process connected peripherals
        auto connected = pool.process();

        // Visual indicator of number of devices connected
        if (connected == 0) {
            if (millis() - blink_timer > NO_DEVICE_BLINK_INTERVAL) {
                blink_timer = millis();
                blink_state = !blink_state;
                door_lights.setReady(blink_state);
            }
        }

        // Scan for new devices to add
        if (millis() - last_scan > DOSA_SCAN_FREQ) {
            last_scan = millis();

            if (!bt.isScanning()) {
                // Not in scan mode, start the scanner and wait for next run
                bt.scanForService(dosa::bt::svc_sensor);
                return;
            }

            serial.writeln("(" + String(connected) + ") Scanning..", dosa::LogLevel::DEBUG);

            auto device = BLE.available();
            if (device) {
                door_lights.connecting();

                bt.stopScan();  // IMPORTANT: connection won't work if scanning
                pool.add(device);

                // Set notification devices to indicate we have a sensor ready
                door_lights.ready();
                blink_state = true;
            }
        }
    }

   protected:
    DoorContainer container;

    Container& getContainer() override
    {
        return container;
    }
};

}  // namespace dosa::door