#pragma once

#include <dosa.h>

#include "door_container.h"

#define NO_DEVICE_BLINK_INTERVAL 500
#define SENSOR_TRIGGER_VALUE 2

namespace dosa::door {

class DoorApp final : public dosa::App
{
   public:
    using dosa::App::App;

    void init() override
    {
        App::init();
        container.getDoorLights().ready();
        container.getDevicePool().setDeviceChangeCallback(&sensorStateChangeForwarder, this);
    }

    void loop() override
    {
        auto& serial = container.getSerial();
        auto& bt = container.getBluetooth();
        auto& pool = container.getDevicePool();
        auto& door_lights = container.getDoorLights();

        // Check the door switch
        auto& door_switch = container.getDoorSwitch();
        if (door_switch.process() && door_switch.getState()) {
            serial.writeln("Door switch pressed");
            doorSequence();
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

   private:
    DoorContainer container;

    // Peripheral scan timer
    unsigned long last_scan = 0;

    // When no devices are ready, we'll use this timer to blink the ready LED
    unsigned long blink_timer = 0;
    bool blink_state = true;

    /**
     * Open and close the door, adjust lights in turn.
     */
    void doorSequence()
    {
        container.getDoorLights().activity();
        container.getDoorWinch().trigger();
        container.getDoorLights().ready();
        blink_state = true;
    }

    /**
     * When a peripheral sensor changes state (eg PIR sensor detects motion).
     */
    void sensorStateChange(Sensor& sensor)
    {
        if (sensor.getState() != SENSOR_TRIGGER_VALUE) {
            return;
        }

        container.getSerial().writeln("Sensor triggered");
        doorSequence();
    }

    Container& getContainer() override
    {
        return container;
    }

    /**
     * Context forwarder for device pool callback.
     *
     * Translates a void* context to a DoorApp.
     */
    static void sensorStateChangeForwarder(Sensor& sensor, void* context)
    {
        static_cast<DoorApp*>(context)->sensorStateChange(sensor);
    }
};

}  // namespace dosa::door