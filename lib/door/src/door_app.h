#pragma once

#include <dosa.h>

#include "door_container.h"

#define NO_DEVICE_BLINK_INTERVAL 500  // Blink speed for no-device light
#define DEVICE_INDICATOR_DELAY 10000  // Frequency to display number of sensor devices connected
#define SENSOR_TRIGGER_VALUE 2
#define MAX_SENSORS 2

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
        container.getDoorSwitch().setCallback(&doorSwitchStateChangeForwarder, this);
        container.getDoorWinch().setErrorCallback(&doorWinchErrorForwarder, this);
        container.getDoorWinch().setInterruptCallback(&doorInterruptForwarder, this);
    }

    void loop() override
    {
        auto& serial = container.getSerial();
        auto& bt = container.getBluetooth();
        auto& pool = container.getDevicePool();
        auto& door_lights = container.getDoorLights();

        // Check the door switch
        container.getDoorSwitch().process();

        // Process connected peripherals
        auto connected = pool.process();
        if (connected < last_connected) {
            // Display disconnect indicator
            for (unsigned int i = 0; i < 3; ++i) {
                door_lights.setError(true);
                delay(100);
                door_lights.setError(false);
                delay(100);
            }
        }
        last_connected = connected;

        // Visual indicator of number of devices connected
        if (connected == 0) {
            if (millis() - device_indicator_time > NO_DEVICE_BLINK_INTERVAL) {
                device_indicator_time = millis();
                blink_state = !blink_state;
                door_lights.setReady(blink_state);
            }
        } else {
            if (millis() - device_indicator_time > DEVICE_INDICATOR_DELAY) {
                device_indicator_time = millis();
                for (unsigned int i = 0; i < connected; ++i) {
                    door_lights.setActivity(true);
                    delay(100);
                    door_lights.setActivity(false);
                    delay(100);
                }
            }
        }

        // Scan for new devices to add
        if (connected < MAX_SENSORS && (millis() - last_scan > DOSA_SCAN_FREQ)) {
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
                if (!pool.add(device)) {
                    // Create an error signal
                    door_lights.error();
                    delay(1000);
                }

                door_lights.ready();
                blink_state = true;
                device_indicator_time = millis();
            }
        }
    }

   private:
    DoorContainer container;

    // Peripheral scan timer
    unsigned long last_scan = 0;

    // Indicator light timing controls
    unsigned long device_indicator_time = 0;
    bool blink_state = true;
    unsigned int last_connected = 0;

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

    Container& getContainer() override
    {
        return container;
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
    /**
     * Context forwarder for device pool callback.
     */
    static void sensorStateChangeForwarder(Sensor& sensor, void* context)
    {
        static_cast<DoorApp*>(context)->sensorStateChange(sensor);
    }

    /**
     * When the door switch (blue button) changes state.
     */
    void doorSwitchStateChange(bool state)
    {
        if (!state) {
            return;
        }

        container.getSerial().writeln("Door switch pressed");
        doorSequence();
    }

    /**
     * Context forwarder for door switch callback.
     */
    static void doorSwitchStateChangeForwarder(bool state, void* context)
    {
        static_cast<DoorApp*>(context)->doorSwitchStateChange(state);
    }

    /**
     * Resets device state from an error state.
     */
    void reset()
    {
        auto& lights = container.getDoorLights();
        for (unsigned short i = 0; i < 3; ++i) {
            lights.set(false, false, false, true);
            delay(100);
            lights.set(false, false, true, false);
            delay(100);
            lights.set(false, true, false, false);
            delay(100);
        }

        while (container.getDoorSwitch().getStatePassiveProcess()) {
            lights.off();
            delay(100);
            lights.set(false, true, false, false);
            delay(100);
        }

        container.getSerial().writeln("Reset from error state");
        lights.setSwitch(true);
        blink_state = true;
    }

    /**
     * Creates a holding pattern when the door winch fails.
     */
    void doorErrorHoldingPattern(DoorErrorCode error)
    {
        auto& lights = container.getDoorLights();
        lights.error();

        switch (error) {
            default:
                // Unknown error sequence: all lights blink together
                while (true) {
                    lights.set(false, true, true, true);
                    delay(500);
                    lights.off();
                    delay(500);
                    if (container.getDoorSwitch().getStatePassiveProcess()) {
                        reset();
                        return;
                    }
                }
            case DoorErrorCode::OPEN_TIMEOUT:
                // Open timeout sequence: error solid; activity blinks
                while (true) {
                    lights.setActivity(true);
                    delay(500);
                    lights.setActivity(false);
                    delay(500);
                    if (container.getDoorSwitch().getStatePassiveProcess()) {
                        reset();
                        return;
                    }
                }
            case DoorErrorCode::CLOSE_TIMEOUT:
                // Close timeout sequence: error solid; ready blinks
                while (true) {
                    lights.setReady(true);
                    delay(500);
                    lights.setReady(false);
                    delay(500);
                    if (container.getDoorSwitch().getStatePassiveProcess()) {
                        reset();
                        return;
                    }
                }
            case DoorErrorCode::JAMMED:
                // Jam sequence: error solid; activity/ready alternate
                while (true) {
                    lights.setReady(true);
                    lights.setActivity(false);
                    delay(500);
                    lights.setReady(false);
                    lights.setActivity(true);
                    delay(500);
                    if (container.getDoorSwitch().getStatePassiveProcess()) {
                        reset();
                        return;
                    }
                }
        }
    }

    /**
     * Check if we should interrupt the closing sequence.
     */
    bool doorInterruptCheck()
    {
        return container.getDoorSwitch().getStatePassiveProcess() ||
               container.getDevicePool().passiveStateCheck(SENSOR_TRIGGER_VALUE);
    }

    /**
     * Context forwarder for winch error callback.
     */
    static void doorWinchErrorForwarder(DoorErrorCode error, void* context)
    {
        static_cast<DoorApp*>(context)->doorErrorHoldingPattern(error);
    }

    /**
     * Context forwarder for door interrupt callback.
     */
    static bool doorInterruptForwarder(void* context)
    {
        return static_cast<DoorApp*>(context)->doorInterruptCheck();
    }
};

}  // namespace dosa::door