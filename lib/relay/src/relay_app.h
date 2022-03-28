#pragma once

#include <dosa_ota.h>

#include "const.h"

namespace dosa {

class RelayApp final : public dosa::OtaApplication
{
   public:
    using dosa::OtaApplication::OtaApplication;

    void init() override
    {
        OtaApplication::init();

        container.getComms().newHandler<comms::StandardHandler<messages::Trigger>>(
            DOSA_COMMS_MSG_TRIGGER,
            &triggerMessageForwarder,
            this);

        pinMode(DOSA_RELAY_PIN, OUTPUT);
        digitalWrite(DOSA_RELAY_PIN, LOW);
        relay_last_moved = millis();
    }

    void loop() override
    {
        OtaApplication::loop();

        // Check if we want to deactivate following a time-delay activation
        auto switch_delay = getContainer().getSettings().getRelayActivationTime();
        if (switch_delay > 0 && relay_state && (millis() - relay_last_moved > switch_delay)) {
            setRelay(false);
        }
    }

   private:
    Container container;
    uint16_t last_msg_id = 0;
    bool relay_state = false;
    uint32_t relay_last_moved = 0;

    void setRelay(bool state)
    {
        relay_last_moved = millis();

        if (relay_state == state) {
            return;
        }

        logln(String("Set power state: ") + (state ? "active" : "inactive"));
        relay_state = state;

        if (relay_state) {
            digitalWrite(DOSA_RELAY_PIN, HIGH);
            setDeviceState(messages::DeviceState::WORKING);
            dispatchGenericMessage(DOSA_COMMS_MSG_BEGIN);
            getStats().count(stats::begin);
        } else {
            digitalWrite(DOSA_RELAY_PIN, LOW);
            setDeviceState(messages::DeviceState::OK);
            dispatchGenericMessage(DOSA_COMMS_MSG_END);
            getStats().count(stats::end);
        }
    }

    void onDebugRequest(messages::GenericMessage const& msg, comms::Node const& sender) override
    {
        App::onDebugRequest(msg, sender);
        auto const& settings = getContainer().getSettings();
        netLog("Relay activation time: " + String(settings.getRelayActivationTime()), sender);
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
     * Toggles the state of the relay, based on the application settings.
     */
    void executePowerToggle()
    {
        if (getContainer().getSettings().getRelayActivationTime() > 0) {
            // Time-delay mode
            setRelay(true);
        } else {
            // Toggle mode
            setRelay(!relay_state);
        }
    }

    /**
     * Sensor has broadcasted a trigger event.
     */
    void onTrigger(messages::Trigger const& trigger, comms::Node const& sender)
    {
        // Don't activate if this message is duplicate
        if (last_msg_id == trigger.getMessageId()) {
            logln(
                "Duplicate trigger detected from '" + Comms::getDeviceName(trigger) + "' (" +
                    comms::nodeToString(sender) + "), msg ID: " + String(trigger.getMessageId()),
                LogLevel::DEBUG);
            return;
        } else {
            last_msg_id = trigger.getMessageId();
        }

        String sender_name = Comms::getDeviceName(trigger);
        String sender_str = "'" + sender_name + "' (" + comms::nodeToString(sender) + ")";
        auto const& settings = getContainer().getSettings();

        if (!settings.isListenForAllDevices() && !settings.hasListenDevice(sender_name)) {
            logln("Ignoring trigger from " + sender_str, LogLevel::DEBUG);
            return;
        } else if (isLocked()) {
            switch (getLockState()) {
                default:
                case LockState::LOCKED:
                    logln("Ignoring trigger while device is locked");
                    getStats().count(stats::sec_locked);
                    break;
                case LockState::ALERT:
                    netLog("Lock violation by " + sender_str, NetLogLevel::SECURITY);
                    getStats().count(stats::sec_alert);
                    break;
                case LockState::BREACH:
                    // This lock state makes little sense for a relay, but we'll alter the message a little
                    netLog("Lock breach by " + sender_str, NetLogLevel::SECURITY);
                    getStats().count(stats::sec_breached);
                    break;
            }
            return;
        } else {
            logln("Executing trigger from " + sender_str);
        }

        // Send reply ack
        container.getComms().dispatch(sender, messages::Ack(trigger, container.getSettings().getDeviceNameBytes()));

        // Physically toggle the relay
        executePowerToggle();
    }
    /**
     *
     * Context forwarder for trigger messages.
     */
    static void triggerMessageForwarder(messages::Trigger const& trigger, comms::Node const& sender, void* context)
    {
        static_cast<RelayApp*>(context)->onTrigger(trigger, sender);
    }
};

}  // namespace dosa
