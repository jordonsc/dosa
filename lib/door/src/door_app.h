#pragma once

#include <dosa_ota.h>

#include "door_container.h"

#define DOSA_DOOR_ERR_UNKNOWN "Door unknown error"
#define DOSA_DOOR_ERR_OPEN "Door OPEN timeout"
#define DOSA_DOOR_ERR_CLOSE "Door CLOSE timeout"
#define DOSA_DOOR_ERR_JAM "Door JAMMED"
#define DOSA_DOOR_ERR_SONAR "Sonar fault, falling back to legacy mode"
#define DOSA_DOOR_ERR_CALIBRATE_TO "Calibration timeout"

namespace dosa {

class DoorApp final : public dosa::OtaApplication
{
   public:
    using dosa::OtaApplication::OtaApplication;

    explicit DoorApp(Config config) : OtaApplication(std::move(config)) {}

    void init() override
    {
        OtaApplication::init();

        container.getDoorLights().ready();
        container.getDoorSwitch().setCallback(&doorSwitchStateChangeForwarder, this);
        container.getDoorWinch().setErrorCallback(&doorWinchErrorForwarder, this);
        container.getDoorWinch().setInterruptCallback(&doorInterruptForwarder, this);
        container.getDoorWinch().setTickCallback(&doorTickForwarder, this);

        container.getComms().newHandler<comms::StandardHandler<messages::Trigger>>(
            DOSA_COMMS_MSG_TRIGGER,
            &triggerMessageForwarder,
            this);

        container.getComms().newHandler<comms::StandardHandler<messages::Alt>>(
            DOSA_COMMS_MSG_ALT,
            &altMessageForwarder,
            this);
    }

    void loop() override
    {
        OtaApplication::loop();

        // Check the door switch
        container.getDoorSwitch().process();

        // Check if the wifi handler has picked up a trigger request
        if (!isErrorState()) {
            if (door_fire_from_udp) {
                // Check for primary trigger
                doorSequence();

                // Clear out any pending trigger messages
                while (getContainer().getComms().processInbound()) {
                }
                door_fire_from_udp = false;
            } else if (rewind_request > 0) {
                // Check for rewind request (close by fractional amount + recalibrate)
                rewindSequence();

                while (getContainer().getComms().processInbound()) {
                }
                rewind_request = 0;
            }
        }

        /**
         * Important: we need to keep reading from the serial interface so that we don't get stale data when the door
         *            sequence is triggered.
         */
        container.getSonar().process();
    }

   private:
    DoorContainer container;
    bool door_fire_from_udp = false;   // Wifi request to open the door, sets a flag for the next loop
    unsigned long rewind_request = 0;  // Alt-trigger to close door and recalibrate

    void onDebugRequest(messages::GenericMessage const& msg, comms::Node const& sender) override
    {
        if (msg_cache.validate(sender, msg.getMessageId())) {
            return;
        }

        App::onDebugRequest(msg, sender);
        netLog("Open-stop distance: " + String(getContainer().getSettings().getDoorOpenDistance()), sender);
        netLog("Open-wait: " + String(getContainer().getSettings().getDoorOpenWait()), sender);
        netLog("Close ticks: " + String(getContainer().getSettings().getDoorCloseTicks()), sender);
        netLog("Cool-down: " + String(getContainer().getSettings().getDoorCoolDown()), sender);
        netLog("Sonar distance: " + String(container.getSonar().getDistance()), sender);
    }

    /**
     * Sensor has broadcasted a trigger event.
     */
    void onTrigger(messages::Trigger const& trigger, comms::Node const& sender)
    {
        if (msg_cache.validate(sender, trigger.getMessageId())) {
            return;
        }

        // Set a flag that informs the main loop to open the door
        if (canTrigger(trigger, sender)) {
            netLog("Trigger by network", NetLogLevel::INFO);
            door_fire_from_udp = true;
        }
    }

    /**
     * Sensor has broadcasted an alt event.
     */
    void onAlt(messages::Alt const& alt, comms::Node const& sender)
    {
        if (msg_cache.validate(sender, alt.getMessageId())) {
            return;
        }

        // Set a flag that informs the main loop to open the door
        if (canTrigger(alt, sender)) {
            if (alt.getCode() > 0 && alt.getCode() < 11) {
                // Rewind request - "close" the door to the scale of alt.getCode()
                rewind_request = getSettings().getDoorCloseTicks() * alt.getCode() / 10;
                netLog(
                    "Rewind requested, code: " + String(alt.getCode()) + "; ticks: " + String(rewind_request),
                    NetLogLevel::INFO);
            } else {
                netLog("Ignoring unknown alt-trigger: " + String(alt.getCode()), NetLogLevel ::WARNING);
            }
        }
    }

    /**
     * Open and close the door, adjust lights in turn.
     */
    void doorSequence()
    {
        if (isLocked()) {
            // Locks should be checked at their trigger-point, this is a final just-in-case
            return;
        }

        setDeviceState(messages::DeviceState::WORKING);
        getStats().count(stats::begin);
        auto start = millis();
        dispatchGenericMessage(DOSA_COMMS_MSG_BEGIN, true);

        container.getDoorLights().activity();
        container.getDoorWinch().trigger();
        container.getDoorLights().ready();

        if (!isErrorState()) {
            setDeviceState(messages::DeviceState::OK);
        }
        getStats().timing(stats::sequence, millis() - start);
        getStats().count(stats::end);
        dispatchGenericMessage(DOSA_COMMS_MSG_END, true);
    }

    /**
     * Fractional door close and recalibrate
     */
    void rewindSequence()
    {
        setDeviceState(messages::DeviceState::WORKING);
        getStats().count(stats::begin);
        auto start = millis();
        dispatchGenericMessage(DOSA_COMMS_MSG_BEGIN, true);

        container.getDoorLights().activity();
        container.getDoorWinch().rewind(rewind_request);
        container.getDoorLights().ready();

        if (!isErrorState()) {
            setDeviceState(messages::DeviceState::OK);
        }

        getStats().timing(stats::alt, millis() - start);
        getStats().count(stats::end);
        dispatchGenericMessage(DOSA_COMMS_MSG_END, true);
    }

    Container& getContainer() override
    {
        return container;
    }

    Container const& getContainer() const override
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

        if (isLocked()) {
            netLog("Lock violation by physical button press", NetLogLevel::WARNING);
            switch (getLockState()) {
                default:
                case LockState::ALERT:
                    secAlert(SecurityLevel::ALERT);
                    break;
                case LockState::BREACH:
                    secAlert(SecurityLevel::BREACH);
                    break;
            }
            return;
        }

        netLog("Trigger by physical button", NetLogLevel::INFO);
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
     * Door winch has failed, report and set device to an error mode.
     */
    void setDoorErrorCondition(DoorErrorCode error)
    {
        switch (error) {
            default:
                bt_error_msg.setValue(DOSA_DOOR_ERR_UNKNOWN);
                netLog(DOSA_DOOR_ERR_UNKNOWN, NetLogLevel::CRITICAL);
                setDeviceState(messages::DeviceState::UNKNOWN);
                break;

            case DoorErrorCode::OPEN_TIMEOUT:
                bt_error_msg.setValue(DOSA_DOOR_ERR_OPEN);
                netLog(DOSA_DOOR_ERR_OPEN, NetLogLevel::ERROR);
                setDeviceState(messages::DeviceState::MINOR_FAULT);
                break;

            case DoorErrorCode::CLOSE_TIMEOUT:
                bt_error_msg.setValue(DOSA_DOOR_ERR_CLOSE);
                netLog(DOSA_DOOR_ERR_CLOSE, NetLogLevel::CRITICAL);
                setDeviceState(messages::DeviceState::MAJOR_FAULT);
                break;

            case DoorErrorCode::JAMMED:
                bt_error_msg.setValue(DOSA_DOOR_ERR_JAM);
                netLog(DOSA_DOOR_ERR_JAM, NetLogLevel::CRITICAL);
                setDeviceState(messages::DeviceState::CRITICAL);
                break;

            case DoorErrorCode::SONAR_ERROR:
                netLog(DOSA_DOOR_ERR_SONAR, NetLogLevel::ERROR);
                setDeviceState(messages::DeviceState::MINOR_FAULT);
                break;

            case DoorErrorCode::CALIBRATE_TIMEOUT:
                netLog(DOSA_DOOR_ERR_CALIBRATE_TO, NetLogLevel::ERROR);
                setDeviceState(messages::DeviceState::MINOR_FAULT);
                break;
        }
    }

    /**
     * Check if we should interrupt the closing sequence.
     */
    bool doorInterruptCheck()
    {
        // Check for UDP 'trg' packets - we'll disable the open-door-request flag and instead return an interrupt
        // to the active winch loop.
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
     * Run continuously while the door is operating. This allows us to process inbound traffic, return acks, etc.
     */
    void doorTick()
    {
        if (isWifiConnected()) {
            getContainer().getComms().processInbound();

            // Door is already active, don't attempt to start the open sequence.
            if (door_fire_from_udp) {
                door_fire_from_udp = false;
            }
        }
    }

    /**
     * Context forwarder for winch tick callback.
     */
    static void doorTickForwarder(void* context)
    {
        static_cast<DoorApp*>(context)->doorTick();
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

    /**
     * Context forwarder for alt messages.
     */
    static void altMessageForwarder(messages::Alt const& trigger, comms::Node const& sender, void* context)
    {
        static_cast<DoorApp*>(context)->onAlt(trigger, sender);
    }
};

}  // namespace dosa
