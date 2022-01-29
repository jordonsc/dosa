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

        container.getComms().newHandler<comms::StandardHandler<messages::Trigger>>(
            DOSA_COMMS_TRIGGER_MSG_CODE,
            &triggerMessageForwarder,
            this);
    }

    void loop() override
    {
        App::loop();

        // Check the door switch
        container.getDoorSwitch().process();

        // Check if the wifi handler has picked up an trigger request
        if (door_fire_from_udp && !error_state) {
            doorSequence();

            // Clear out any pending trigger messages
            while (getContainer().getComms().processInbound()) {
            }
            door_fire_from_udp = false;
        }
    }

   private:
    DoorContainer container;
    uint16_t last_msg_id = 0;
    bool error_state = false;  // If there has been an error, don't trigger from wifi until the button has been pressed
    bool door_fire_from_udp = false;  // Wifi request to open the door, sets a flag for the next loop

    /**
     * Sensor has broadcasted a trigger event.
     */
    void onTrigger(messages::Trigger const& trigger, comms::Node const& sender)
    {
        // Don't open the door if this message is duplicate
        if (last_msg_id == trigger.getMessageId()) {
            logln(
                "Duplicate trigger detected from '" + Comms::getDeviceName(trigger) + "' (" +
                    comms::nodeToString(sender) + "), msg ID: " + String(trigger.getMessageId()),
                LogLevel::DEBUG);
            return;
        } else {
            last_msg_id = trigger.getMessageId();
        }

        container.getSerial().writeln(
            "Received trigger message from '" + Comms::getDeviceName(trigger) + "' (" + comms::nodeToString(sender) +
            "), msg ID: " + String(trigger.getMessageId()));

        // Send reply ack
        container.getComms().dispatch(sender, messages::Ack(trigger, container.getSettings().getDeviceNameBytes()));

        // Set a flag that informs the main loop to open the door, or the interrupt handler
        door_fire_from_udp = true;
    }

    /**
     * Wifi connection established, bind UDP multicast.
     */
    void onWifiConnect() override
    {
        App::onWifiConnect();

        if (container.getComms().bindMulticast(comms::multicastAddr)) {
            container.getSerial().writeln("Listening for multicast packets");
            dispatchGenericMessage(DOSA_COMMS_ONLINE);
        } else {
            container.getSerial().writeln("Failed to bind multicast", LogLevel::ERROR);
        }
    }

    /**
     * Open and close the door, adjust lights in turn.
     */
    void doorSequence()
    {
        dispatchGenericMessage(DOSA_COMMS_BEGIN);
        container.getDoorLights().activity();
        container.getDoorWinch().trigger();
        container.getDoorLights().ready();
        dispatchGenericMessage(DOSA_COMMS_END);
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
        error_state = false;  // button will reset error state
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
    void setDoorErrorCondition(DoorErrorCode error)
    {
        auto& lights = container.getDoorLights();

        lights.error();
        error_state = true;

        container.getComms().dispatch(
            comms::multicastAddr,
            messages::GenericMessage(DOSA_COMMS_ERROR, container.getSettings().getDeviceNameBytes()),
            false);

        switch (error) {
            default:
                // Unknown error sequence: all lights blink together
                bt_error_msg.setValue("Door unknown error");

            case DoorErrorCode::OPEN_TIMEOUT:
                // Open timeout sequence: error solid; activity blinks
                bt_error_msg.setValue("Door OPEN timeout");
                break;

            case DoorErrorCode::CLOSE_TIMEOUT:
                // Close timeout sequence: error solid; ready blinks
                bt_error_msg.setValue("Door CLOSE timeout");
                break;

            case DoorErrorCode::JAMMED:
                // Jam sequence: error solid; activity/ready alternate
                bt_error_msg.setValue("Door JAMMED");
                break;
        }
    }

    /**
     * Check if we should interrupt the closing sequence.
     */
    bool doorInterruptCheck()
    {
        // Check for UDP 'trg' packets - we'll disable the open flag and instead return an interrupt
        if (isWifiConnected()) {
            getContainer().getComms().processInbound();

            if (door_fire_from_udp) {
                door_fire_from_udp = false;
                return true;
            }
        }

        // else check the door switch
        return container.getDoorSwitch().getStatePassiveProcess();
    }

    /**
     * Context forwarder for winch error callback.
     */
    static void doorWinchErrorForwarder(DoorErrorCode error, void* context)
    {
        static_cast<DoorApp*>(context)->setDoorErrorCondition(error);
    }

    /**
     * Context forwarder for door interrupt callback.
     */
    static bool doorInterruptForwarder(void* context)
    {
        return static_cast<DoorApp*>(context)->doorInterruptCheck();
    }

    /**
     * Context forwarder for trigger messages.
     */
    static void triggerMessageForwarder(messages::Trigger const& trigger, comms::Node const& sender, void* context)
    {
        static_cast<DoorApp*>(context)->onTrigger(trigger, sender);
    }
};

}  // namespace dosa::door
