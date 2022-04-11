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
        activity_led.on();
        alert_led.on();

        OtaApplication::init();

        container.getComms().newHandler<comms::StandardHandler<messages::Trigger>>(
            DOSA_COMMS_MSG_TRIGGER,
            &triggerMessageForwarder,
            this);

        container.getComms().newHandler<comms::StandardHandler<messages::LogMessage>>(
            DOSA_COMMS_MSG_LOG,
            &logMessageForwarder,
            this);

        container.getComms().newHandler<comms::StandardHandler<messages::Security>>(
            DOSA_COMMS_MSG_SECURITY,
            &secMessageForwarder,
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

    void onWifiConnect() override
    {
        App::onWifiConnect();
        activity_led.off();
        alert_led.off();
    }

    void onWifiDisconnect() override
    {
        App::onWifiDisconnect();
        activity_led.on();
        alert_led.on();
    }

   private:
    Container container;
    Light activity_led;
    Light alert_led;
    Switch button;

    void onDebugRequest(messages::GenericMessage const& msg, comms::Node const& sender) override
    {
        if (msg_cache.validate(sender, msg.getMessageId())) {
            return;
        }

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
        if (msg_cache.validate(sender, trigger.getMessageId())) {
            return;
        }

        if (canTrigger(trigger, sender)) {
            activity_led.begin(DOSA_ACTIVITY_DURATION);
        }
    }

    /**
     * Log message received
     */
    void onLog(messages::LogMessage const& log, comms::Node const& sender)
    {
        if (msg_cache.validate(sender, log.getMessageId())) {
            return;
        }

        if (log.getLogLevel() < messages::LogMessageLevel::ERROR) {
            return;
        }

        String sender_name = Comms::getDeviceName(log);
        String sender_str = "'" + sender_name + "' (" + comms::nodeToString(sender) + ")";
        logln("Error alert from " + sender_str);

        alert_led.setSequence(DOSA_ERROR_SEQ_ON, DOSA_ERROR_SEQ_OFF);
        alert_led.begin(DOSA_ALERT_DURATION);
    }

    /**
     * Security message received
     */
    void onSecurity(messages::Security const& sec, comms::Node const& sender)
    {
        if (msg_cache.validate(sender, sec.getMessageId())) {
            return;
        }

        String sender_name = Comms::getDeviceName(sec);
        String sender_str = "'" + sender_name + "' (" + comms::nodeToString(sender) + ")";
        logln("Security alert from " + sender_str + " (" + String(sec.getMessageId()) + ")");

        switch (sec.getSecurityLevel()) {
            default:
            case SecurityLevel::TAMPER:
            case SecurityLevel::ALERT:
                alert_led.setSequence(DOSA_ALERT_SEQ_ON, DOSA_ALERT_SEQ_OFF);
                break;
            case SecurityLevel::BREACH:
            case SecurityLevel::PANIC:
                alert_led.setSequence(DOSA_BREACH_SEQ_ON, DOSA_BREACH_SEQ_OFF);
                break;
        }

        alert_led.begin(DOSA_ALERT_DURATION);
    }

    /**
     * Flush message received
     */
    void onFlush(messages::GenericMessage const& msg, comms::Node const& sender)
    {
        if (msg_cache.validate(sender, msg.getMessageId())) {
            return;
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
            if (isLocked()) {
                logln("Ignoring user-triggered stand-down request while locked!");
                return;
            }

            logln("Button press: stand down & flush network");

            alert_led.end();
            activity_led.end();

            dispatchMessage(messages::GenericMessage(DOSA_COMMS_MSG_FLUSH, getDeviceNameBytes()), false);
        } else {
            logln("Button press: alarm");

            alert_led.setSequence(DOSA_BREACH_SEQ_ON, DOSA_BREACH_SEQ_OFF);
            alert_led.begin(0);

            getStats().count(stats::sec_panic);
            secAlert(SecurityLevel::PANIC);
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
     * Context forwarder for security messages.
     */
    static void secMessageForwarder(messages::Security const& sec, comms::Node const& sender, void* context)
    {
        static_cast<AlarmApp*>(context)->onSecurity(sec, sender);
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
