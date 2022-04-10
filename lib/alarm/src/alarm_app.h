#pragma once

#include <dosa_ota.h>

#include "const.h"

namespace dosa {

class AlarmApp final : public dosa::OtaApplication
{
   public:
    AlarmApp(Config cfg)
        : OtaApplication(std::move(cfg)),
          activity_led(DOSA_LED_ACTIVITY, DOSA_ACTIVITY_SEQ_ON, DOSA_ACTIVITY_SEQ_OFF),
          alert_led(DOSA_LED_ALERT),
          button(DOSA_ALERT_BUTTON, true)
    {}

    void init() override
    {
        OtaApplication::init();

        container.getComms().newHandler<comms::StandardHandler<messages::Trigger>>(
            DOSA_COMMS_MSG_TRIGGER,
            &triggerMessageForwarder,
            this);

        container.getComms().newHandler<comms::StandardHandler<messages::LogMessage>>(
            DOSA_COMMS_MSG_LOG,
            &logMessageForwarder,
            this);

        container.getComms().newHandler<comms::StandardHandler<messages::GenericMessage>>(
            DOSA_COMMS_MSG_FLUSH,
            &flushMessageForwarder,
            this);

        button.setCallback(&switchForwarder, this);
    }

    void loop() override
    {
        static uint32_t t = millis();

        OtaApplication::loop();
        activity_led.process();
        alert_led.process();
        button.process();
    }

   private:
    Container container;
    Light activity_led;
    Light alert_led;
    Switch button;

    void onDebugRequest(messages::GenericMessage const& msg, comms::Node const& sender) override
    {
        App::onDebugRequest(msg, sender);
        auto const& settings = getContainer().getSettings();
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
     * Sensor has broadcasted a trigger event.
     */
    void onTrigger(messages::Trigger const& trigger, comms::Node const& sender)
    {
        static uint16_t last_msg_id = 0;

        // Don't activate if this message is duplicate
        if (last_msg_id == trigger.getMessageId()) {
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
        } else {
            logln("Trigger from " + sender_str);
            activity_led.begin(DOSA_ACTIVITY_DURATION);
        }
    }

    /**
     * Log message received
     */
    void onLog(messages::LogMessage const& log, comms::Node const& sender)
    {
        static uint16_t last_msg_id = 0;

        // Ignore anything other than security or error messages
        if (log.getLogLevel() < messages::LogMessageLevel::ERROR) {
            return;
        }

        // Don't activate if this message is duplicate
        if (last_msg_id == log.getMessageId()) {
            return;
        } else {
            last_msg_id = log.getMessageId();
        }

        String sender_name = Comms::getDeviceName(log);
        String sender_str = "'" + sender_name + "' (" + comms::nodeToString(sender) + ")";
        String msg = stringFromBytes(log.getMessage(), log.getMessageSize());

        alert_led.end();

        switch (log.getLogLevel()) {
            case messages::LogMessageLevel::ERROR:
            case messages::LogMessageLevel::CRITICAL:
                logln("Error alert from " + sender_str);
                alert_led.setSequence(DOSA_ERROR_SEQ_ON, DOSA_ERROR_SEQ_OFF);
                break;
            case messages::LogMessageLevel::SECURITY:
                if (msg == DOSA_SEC_SENSOR_BREACH) {
                    logln("Security breach from " + sender_str);
                    alert_led.setSequence(DOSA_BREACH_SEQ_ON, DOSA_BREACH_SEQ_OFF);
                } else {
                    logln("Security alert from " + sender_str);
                    alert_led.setSequence(DOSA_ALERT_SEQ_ON, DOSA_ALERT_SEQ_OFF);
                }
                break;
            default:
                // Shouldn't get here
                return;
        }

        alert_led.begin(DOSA_ALERT_DURATION);
    }

    /**
     * Flush message received
     */
    void onFlush(messages::GenericMessage const& msg, comms::Node const& sender)
    {
        static uint16_t last_msg_id = 0;

        // Don't activate if this message is duplicate
        if (last_msg_id == msg.getMessageId()) {
            return;
        } else {
            last_msg_id = msg.getMessageId();
        }

        if (isLocked()) {
            logln("Flush received but device locked, refusing to stand down.");
            return;
        }

        logln("Flush received, standing down.");

        alert_led.end();
        activity_led.end();
    }

    void onSwitchChange(bool state, uint32_t t)
    {
        if (state) {
            return;
        }

        if (t > DOSA_BUTTON_LONG_PRESS) {
            logln("Button press: stand down");

            alert_led.end();
            activity_led.end();
        } else {
            logln("Button press: alarm");

            alert_led.setSequence(DOSA_BREACH_SEQ_ON, DOSA_BREACH_SEQ_OFF);
            alert_led.begin(0);

            netLog(DOSA_SEC_USER_PANIC, NetLogLevel::SECURITY);
        }
    }

    /**
     *
     * Context forwarder for trigger messages.
     */
    static void triggerMessageForwarder(messages::Trigger const& trigger, comms::Node const& sender, void* context)
    {
        static_cast<AlarmApp*>(context)->onTrigger(trigger, sender);
    }

    /**
     *
     * Context forwarder for log messages.
     */
    static void logMessageForwarder(messages::LogMessage const& log, comms::Node const& sender, void* context)
    {
        static_cast<AlarmApp*>(context)->onLog(log, sender);
    }

    /**
     *
     * Context forwarder for flush messages.
     */
    static void flushMessageForwarder(messages::GenericMessage const& msg, comms::Node const& sender, void* context)
    {
        static_cast<AlarmApp*>(context)->onFlush(msg, sender);
    }

    /**
     * Context forwarded for button presses.
     */
    static void switchForwarder(bool state, uint32_t t, void* context)
    {
        static_cast<AlarmApp*>(context)->onSwitchChange(state, t);
    }
};

}  // namespace dosa
