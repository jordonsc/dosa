#pragma once

#include <dosa_ota.h>

#include "const.h"

// If defined, the holding the button will send a flush along with clearing device state
#define DOSA_ALARM_BTN_FLUSH

namespace dosa {

enum class AlertLevel : uint8_t
{
    NONE,
    ERROR,
    ALERT,
    BREACH
};

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

        container.getComms().newHandler<comms::StandardHandler<messages::GenericMessage>>(
            DOSA_COMMS_MSG_BEGIN,
            &beginEndMessageForwarder,
            this);

        container.getComms().newHandler<comms::StandardHandler<messages::GenericMessage>>(
            DOSA_COMMS_MSG_END,
            &beginEndMessageForwarder,
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

        logln("Begin LinkedList test");
        ll_settings.set(CfgItem::DEVICE_NAME, "Foo");
        ll_settings.set(CfgItem::PASSWORD, String("Bar"));
        ll_settings.set(CfgItem::STATS_SVR_PORT, uint16_t(1234));

        if (ll_settings[CfgItem::DEVICE_NAME].getDataType() == LinkedListItemType::STRING) {
            logln("Device Name: " + ll_settings[CfgItem::DEVICE_NAME].getString());
        } else {
            logln("Incorrect data type for Device Name");
        }

        if (ll_settings[CfgItem::PASSWORD].getDataType() == LinkedListItemType::STRING) {
            logln("Password: " + ll_settings[CfgItem::PASSWORD].getString());
        } else {
            logln("Incorrect data type for Password");
        }

        if (ll_settings[CfgItem::STATS_SVR_PORT].getDataType() == LinkedListItemType::UINT16) {
            logln("Stats Port: " + String(ll_settings[CfgItem::STATS_SVR_PORT].getUInt16()));
        } else {
            logln("Incorrect data type for Stats Port");
        }

        for (auto const& item : ll_settings) {
            switch (item.getDataType()) {
                case LinkedListItemType::NONE:
                    logln("> None");
                    break;
                case LinkedListItemType::PTR:
                    logln("> Pointer");
                    break;
                case LinkedListItemType::BOOL:
                    logln("> " + String(item.getBool() ? "True" : "False"));
                    break;
                case LinkedListItemType::STRING:
                    logln("> " + item.getString());
                    break;
                case LinkedListItemType::UINT8:
                    logln("> " + String(item.getUInt8()));
                    break;
                case LinkedListItemType::UINT16:
                    logln("> " + String(item.getUInt16()));
                    break;
                case LinkedListItemType::UINT32:
                    logln("> " + String(item.getUInt32()));
                    break;
                case LinkedListItemType::INT8:
                    logln("> " + String(item.getInt8()));
                    break;
                case LinkedListItemType::INT16:
                    logln("> " + String(item.getInt16()));
                    break;
                case LinkedListItemType::INT32:
                    logln("> " + String(item.getInt32()));
                    break;
                case LinkedListItemType::FLOAT32:
                    logln("> " + String(item.getFloat32()));
                    break;
                case LinkedListItemType::FLOAT64:
                    logln("> " + String(item.getFloat64()));
                    break;
            }
        }

        logln("end test");
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
    AlertLevel alert_level = AlertLevel::NONE;
    LinkedList<CfgItem> ll_settings;

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
     * Sensor has broadcasted a begin or end event.
     */
    void onBeginEnd(messages::GenericMessage const& msg, comms::Node const& sender)
    {
        if (msg_cache.validate(sender, msg.getMessageId())) {
            return;
        }

        if (canTrigger(msg, sender)) {
            String cmd = stringFromBytes(msg.getCommandCode(), 3);
            if (cmd == DOSA_COMMS_MSG_BEGIN) {
                activity_led.begin(0);
            } else if (cmd == DOSA_COMMS_MSG_END) {
                activity_led.end();
            } else {
                // Should never get here
                logln("Unknown command code '" + cmd + "' from " + Comms::getDeviceName(msg), LogLevel::WARNING);
            }
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

        setAlertLevel(AlertLevel::ERROR);
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
                setAlertLevel(AlertLevel::ALERT);
                break;
            case SecurityLevel::BREACH:
            case SecurityLevel::PANIC:
                setAlertLevel(AlertLevel::BREACH);
                break;
        }
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
        alert_level = AlertLevel::NONE;
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
            alert_level = AlertLevel::NONE;

#ifdef DOSA_ALARM_BTN_FLUSH
            dispatchMessage(messages::GenericMessage(DOSA_COMMS_MSG_FLUSH, getDeviceNameBytes()), false);
#endif
        } else {
            logln("Button press: alarm");

            getStats().count(stats::sec_panic);
            secAlert(SecurityLevel::PANIC);
            setAlertLevel(AlertLevel::BREACH);
        }
    }

    void setAlertLevel(AlertLevel level)
    {
        if (level <= alert_level) {
            return;
        }

        switch (level) {
            default:
            case AlertLevel::ERROR:
                alert_led.setSequence(DOSA_ERROR_SEQ_ON, DOSA_ERROR_SEQ_OFF);
                break;
            case AlertLevel::ALERT:
                alert_led.setSequence(DOSA_ALERT_SEQ_ON, DOSA_ALERT_SEQ_OFF);
                break;
            case AlertLevel::BREACH:
                alert_led.setSequence(DOSA_BREACH_SEQ_ON, DOSA_BREACH_SEQ_OFF);
                break;
        }

        alert_led.begin(0);
        alert_level = level;
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
     * Context forwarder for begin/end messages.
     */
    static void beginEndMessageForwarder(messages::GenericMessage const& msg, comms::Node const& sender, void* context)
    {
        static_cast<AlarmApp*>(context)->onBeginEnd(msg, sender);
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
