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
    bool relay_state = false;
    uint32_t relay_last_moved = 0;
    uint32_t relay_open_time = 0;

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
            getStats().count(stats::begin);
            relay_open_time = millis();
            dispatchGenericMessage(DOSA_COMMS_MSG_BEGIN, true);
        } else {
            digitalWrite(DOSA_RELAY_PIN, LOW);
            setDeviceState(messages::DeviceState::OK);
            getStats().count(stats::end);
            if (relay_open_time != 0) {
                getStats().timing(stats::sequence, millis() - relay_open_time);
            }
            dispatchGenericMessage(DOSA_COMMS_MSG_END, true);
            relay_open_time = 0;
        }
    }

    void onDebugRequest(messages::GenericMessage const& msg, comms::Node const& sender) override
    {
        if (msg_cache.validate(sender, msg.getMessageId())) {
            return;
        }

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
        if (msg_cache.validate(sender, trigger.getMessageId())) {
            return;
        }

        // Physically toggle the relay
        if (canTrigger(trigger, sender)) {
            executePowerToggle();
        }
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
