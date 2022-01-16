#pragma once

#include <dosa.h>

#include "door_container.h"

#define SENSOR_TRIGGER_VALUE 2  // "active" state for sensors

namespace dosa::door {

class DoorApp final : public dosa::App
{
   public:
    using dosa::App::App;

    explicit DoorApp(Config config) : App(std::move(config)) {}

    void init() override
    {
        App::init();
        container.getDoorLights().ready();
        container.getDoorSwitch().setCallback(&doorSwitchStateChangeForwarder, this);
        container.getDoorWinch().setErrorCallback(&doorWinchErrorForwarder, this);
        container.getDoorWinch().setInterruptCallback(&doorInterruptForwarder, this);
    }

    void loop() override
    {
        stdLoop();

        // Check the door switch
        container.getDoorSwitch().process();

        // Check for broadcasted sensor signals
        if (millis() - wifi_last_checked > WIFI_CHECK) {
            wifi_last_checked = millis();
            auto& serial = container.getSerial();

            if (container.getWiFi().isConnected()) {
                auto& udp = container.getWiFi().getUdp();
                int packetSize = udp.parsePacket();
                if (packetSize > 0) {
                    serial.writeln("Message: " + udp.readString());
                }
            } else {
                serial.writeln("(Wifi not active)");
            }
        }
    }

   private:
    DoorContainer container;
    unsigned long wifi_last_checked = 0;

    void onWifiConnect() override
    {
        App::onWifiConnect();

        if (container.getWiFi().getUdp().beginMulticast(
                dosa::wifi::sensorBroadcastIp,
                dosa::wifi::sensorBroadcastPort)) {
            container.getSerial().writeln("Listening for multicast packets");
        } else {
            container.getSerial().writeln("Failed to bind multicast");
        }
    }
    /**
     * Open and close the door, adjust lights in turn.
     */
    void doorSequence()
    {
        container.getDoorLights().activity();
        container.getDoorWinch().trigger();
        container.getDoorLights().ready();
    }

    Container& getContainer() override
    {
        return container;
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
        // return container.getDoorSwitch().getStatePassiveProcess() ||
        //        container.getDevicePool().passiveStateCheck(SENSOR_TRIGGER_VALUE);
        return container.getDoorSwitch().getStatePassiveProcess();
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
