#pragma once

#include <dosa.h>

#include "door_container.h"

#define SENSOR_TRIGGER_VALUE 2  // "active" state for sensors

#define DOSA_DOOR_ERR_UNKNOWN "Door unknown error"
#define DOSA_DOOR_ERR_OPEN "Door OPEN timeout"
#define DOSA_DOOR_ERR_CLOSE "Door CLOSE timeout"
#define DOSA_DOOR_ERR_JAM "Door JAMMED"

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
            DOSA_COMMS_MSG_TRIGGER,
            &triggerMessageForwarder,
            this);
    }

    void loop() override
    {
        App::loop();

        // Check the door switch
        container.getDoorSwitch().process();

        // Check if the wifi handler has picked up an trigger request
        if (door_fire_from_udp && !isErrorState()) {
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
     * Open and close the door, adjust lights in turn.
     */
    void doorSequence()
    {
        setDeviceState(messages::DeviceState::WORKING);
        dispatchGenericMessage(DOSA_COMMS_MSG_BEGIN);
        container.getDoorLights().activity();
        container.getDoorWinch().trigger();
        container.getDoorLights().ready();
        setDeviceState(messages::DeviceState::OK);
        dispatchGenericMessage(DOSA_COMMS_MSG_END);
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
     * Creates a holding pattern when the door winch fails.
     */
    void setDoorErrorCondition(DoorErrorCode error)
    {
        auto& lights = container.getDoorLights();

        lights.error();
        setDeviceState(messages::DeviceState::CRITICAL);

        switch (error) {
            default:
                // Unknown error sequence: all lights blink together
                bt_error_msg.setValue(DOSA_DOOR_ERR_UNKNOWN);
                dispatchMessage(messages::Error(DOSA_DOOR_ERR_UNKNOWN, getDeviceNameBytes()));

            case DoorErrorCode::OPEN_TIMEOUT:
                // Open timeout sequence: error solid; activity blinks
                bt_error_msg.setValue(DOSA_DOOR_ERR_OPEN);
                dispatchMessage(messages::Error(DOSA_DOOR_ERR_OPEN, getDeviceNameBytes()));
                break;

            case DoorErrorCode::CLOSE_TIMEOUT:
                // Close timeout sequence: error solid; ready blinks
                bt_error_msg.setValue(DOSA_DOOR_ERR_CLOSE);
                break;

            case DoorErrorCode::JAMMED:
                // Jam sequence: error solid; activity/ready alternate
                bt_error_msg.setValue(DOSA_DOOR_ERR_JAM);
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
