#pragma once

#include <Array.h>
#include <comms.h>
#include <dosa_inkplate.h>

#include "const.h"
#include "dosa_device.h"

#define DOSA_PING_INTERVAL 10000    // how often we send a ping message
#define DOSA_BUTTON_DELAY 3000      // time required before repeating a button press
#define DOSA_MAX_DISPLAY_DEVICES 3  // max number of panels we have space to draw
#define DOSA_NO_CONTACT_TIME 35000  // after 22 seconds (3 pings + buffer), report the device as out of contact
#define DOSA_TRIGGER_WAIT 3000      // time to highlight a triggered sensor
#define DOSA_FORCE_REDRAW 300000    // time to redraw the main screen even if there is no activity (update temp, etc)
#define DOSA_STATUS_TIME 5000       // time a status message appears on the screen

// Status LEDs
#define DOSA_MON_STATUS_GREEN MCP23017_PIN_B1
#define DOSA_MON_STATUS_BLUE MCP23017_PIN_B3
#define DOSA_MON_STATUS_RED MCP23017_PIN_B5

namespace dosa {

int const MAX_DEVICES = 10;

class MonitorApp final : public InkplateApp
{
   public:
    explicit MonitorApp(InkplateConfig const& cfg) : InkplateApp(cfg, INKPLATE_1BIT, &serial) {}

    void init() override
    {
        // UDP message handlers
        getComms().newHandler<comms::StandardHandler<messages::Pong>>(DOSA_COMMS_MSG_PONG, &pongMessageForwarder, this);
        getComms().newHandler<comms::StandardHandler<messages::GenericMessage>>(
            DOSA_COMMS_MSG_ONLINE,
            &onlineMessageForwarder,
            this);
        getComms().newHandler<comms::StandardHandler<messages::GenericMessage>>(
            DOSA_COMMS_MSG_BEGIN,
            &beginMessageForwarder,
            this);
        getComms().newHandler<comms::StandardHandler<messages::GenericMessage>>(
            DOSA_COMMS_MSG_END,
            &endMessageForwarder,
            this);
        getComms().newHandler<comms::StandardHandler<messages::Trigger>>(
            DOSA_COMMS_MSG_TRIGGER,
            &trgMessageForwarder,
            this);

        // This will bring up wifi, when wifi connects it will redraw the main screen
        InkplateApp::init();

        pinMode(16, OUTPUT);
        pinMode(16, HIGH);
    }

    void loop() override
    {
        InkplateApp::loop();
        pingIfRequired();
        auditRegisteredDevices();
        checkButtonPresses();

        if (status_drawn != 0 && (millis() - status_drawn > DOSA_STATUS_TIME)) {
            setStatusMessage("");
            status_drawn = 0;
        }

        if (millis() - last_redraw > DOSA_FORCE_REDRAW) {
            printMain();
            refreshDisplay();
        }

        delay(100);
    }

   private:
    SerialComms serial;
    Array<DosaDevice, MAX_DEVICES> devices;
    uint32_t last_ping = 0;
    uint32_t last_redraw = 0;
    uint32_t button_last_press[3] = {0};
    String status_message{};
    uint32_t status_drawn = 0;

    /**
     * Set the status text message at the bottom of the screen
     */
    void setStatusMessage(String const& message)
    {
        status_message = message;
        status_drawn = millis();
        printMain();
        refreshDisplay();
    }

    /**
     * Send a ping message if the cool-down has expired.
     */
    void pingIfRequired()
    {
        if (millis() - last_ping > DOSA_PING_INTERVAL) {
            checkWifi();
            sendPing();
        }
    }

    void checkButtonPresses()
    {
        /**
         * Button 1: send trigger
         */
        if (getDisplay().readTouchpad(PAD1) && (millis() - button_last_press[0] > DOSA_BUTTON_DELAY)) {
            button_last_press[0] = millis();
            sendTrigger();
        }

        /**
         * Button 2: clear device list and refresh
         */
        if (getDisplay().readTouchpad(PAD2) && (millis() - button_last_press[1] > DOSA_BUTTON_DELAY)) {
            button_last_press[1] = millis();
            devices.clear();
            printMain();
            setDeviceState(messages::DeviceState::OK);
            refreshDisplay(true);
            sendPing();
        }

        /**
         * Button 3: full-screen refresh
         */
        if (getDisplay().readTouchpad(PAD3) && (millis() - button_last_press[2] > DOSA_BUTTON_DELAY)) {
            button_last_press[2] = millis();
            refreshDisplay(true);
        }
    }

    /**
     * Checks if a device has fallen into an error state, or clear a trigger flag.
     */
    void auditRegisteredDevices()
    {
        bool changed = false;

        for (auto& d : devices) {
            if (d.getDeviceState() == messages::DeviceState::TRIGGER &&
                (millis() - d.getStateLastUpdated() > DOSA_TRIGGER_WAIT)) {
                d.setDeviceState(messages::DeviceState::OK);
                changed = true;
            }

            if (millis() - d.getLastContact() > DOSA_NO_CONTACT_TIME &&
                d.getDeviceState() != messages::DeviceState::NOT_RESPONDING) {
                d.setDeviceState(messages::DeviceState::NOT_RESPONDING);
                changed = true;
            }
        }

        if (changed) {
            printMain();
            refreshDisplay();
        }
    }

    /**
     * Checks and reconnects the wifi if it disconnected.
     *
     * May never return if wifi is unavailable. Returns true if this function needed to reconnect.
     */
    bool checkWifi()
    {
        if (!wifi.isConnected()) {
            do {
                printSplash();
                loadingStatus("Reconnecting wifi..");
                if (!connectWifi()) {
                    loadingError("Failed to connect to wifi", false);
                    delay(15000);
                }
            } while (!wifi.isConnected());

            // onWifiConnect() will now clear registered devices and redraw the main screen

            return true;
        } else {
            return false;
        }
    }

    /**
     * Wifi has either come online for the first time, or reconnected.
     */
    void onWifiConnect() override
    {
        // Bind the DOSA UDP multicast group once connected
        logln("Bind multicast..");
        if (!bindMulticast()) {
            logln("Multicast bind failed!", LogLevel::ERROR);
            setDeviceState(messages::DeviceState::MAJOR_FAULT);
        }

        clearDeviceList();
        fullRefreshWhenReady();
    }

    /**
     * Clear the registered device list, and if selected, redraw the main screen (won't refresh).
     */
    void clearDeviceList(bool redraw = true)
    {
        devices.clear();

        if (redraw) {
            printMain();
        }
    }

    /**
     * Dispatch a trigger message, signed a as user button press.
     */
    void sendTrigger()
    {
        logln("Trigger by user action");
        uint8_t map[64] = {0};
        dispatchMessage(messages::Trigger(messages::TriggerDevice::BUTTON, map, getDeviceNameBytes()), true);
    }

    /**
     * Send a ping, we'll listen for ACKs to read device state.
     */
    void sendPing()
    {
        logln("Broadcasting ping..", LogLevel::DEBUG);
        dispatchGenericMessage(DOSA_COMMS_MSG_PING);
        last_ping = millis();
    }

    /**
     * Print the main display.
     *
     * Does NOT request a display update.
     */
    void printMain()
    {
        last_redraw = millis();

        auto& display = getDisplay();
        display.clearDisplay();

        printTemperature();
        printBatteryStatus();

        // Main device panels
        for (uint8_t i = 0; i < (uint8_t)devices.size() && i <= DOSA_MAX_DISPLAY_DEVICES; ++i) {
            printDevice(i, devices[i]);
        }

        // Status message
        if (!status_message.isEmpty()) {
            display.setFont(&DejaVu_Sans_24);
            display.setCursor(30, device_size.height - 30);
            display.print(status_message);
        }
    }

    void printTemperature()
    {
        auto& display = getDisplay();
        // Print the temperature
        display.setFont(&Roboto_90);
        display.setCursor(30, 80);
        display.print(String(display.readTemperature()));
        display.setFont(&DejaVu_Sans_48);
        display.setCursor(display.getCursorX(), 50);
        display.print("C");
    }

    void printBatteryStatus()
    {
        auto& display = getDisplay();

        // Battery voltage
        auto bat = batteryAsPercentage();
        display.setFont(&DejaVu_Sans_48);
        String bat_pc(bat);
        bat_pc += "%";
        printRight(bat_pc.c_str(), device_size.width - 30, 50);

        display.setFont(&DejaVu_Sans_24);
        String voltage(display.readBattery());
        voltage += "v";
        printRight(voltage.c_str(), device_size.width - 30, 80);

        // Battery icon
        char* bat_icon;
        if (bat >= 90) {
            bat_icon = (char*)images::bat_9;
        } else if (bat >= 80) {
            bat_icon = (char*)images::bat_8;
        } else if (bat >= 70) {
            bat_icon = (char*)images::bat_7;
        } else if (bat >= 60) {
            bat_icon = (char*)images::bat_6;
        } else if (bat >= 50) {
            bat_icon = (char*)images::bat_5;
        } else if (bat >= 40) {
            bat_icon = (char*)images::bat_4;
        } else if (bat >= 30) {
            bat_icon = (char*)images::bat_3;
        } else if (bat >= 20) {
            bat_icon = (char*)images::bat_2;
        } else if (bat >= 10) {
            bat_icon = (char*)images::bat_1;
        } else {
            bat_icon = (char*)images::bat_0;
        }
        display.drawPngFromSd(bat_icon, device_size.width - 300, -15, false, false);
    }

    /**
     * Print a device info panel.
     */
    void printDevice(uint8_t pos, DosaDevice const& device)
    {
        auto state = device.getDeviceState();
        int x = 10;
        int y = (pos * (images::panel_size.height + 5)) + 100;  // 5 spacing + 5 top margin + header of 90

        bool inv = state != messages::DeviceState::OK;
        auto bg = inv ? BLACK : WHITE;
        auto fg = inv ? WHITE : BLACK;

        auto& display = getDisplay();
        display.fillRect(x, y, images::panel_size.width, images::panel_size.height, bg);
        display.drawRect(x, y, images::panel_size.width, images::panel_size.height, fg);

        String glyph_fn;
        if (state == messages::DeviceState::CRITICAL || state == messages::DeviceState::MAJOR_FAULT ||
            state == messages::DeviceState::NOT_RESPONDING) {
            glyph_fn = images::error_active;
        } else {
            switch (device.getDeviceType()) {
                default:
                case messages::DeviceType::UNSPECIFIED:
                case messages::DeviceType::MONITOR:
                case messages::DeviceType::UTILITY:
                    glyph_fn = state == messages::DeviceState::OK ? images::misc_inactive : images::misc_active;
                    break;
                case messages::DeviceType::SENSOR_PIR:
                case messages::DeviceType::SENSOR_ACTIVE_IR:
                case messages::DeviceType::SENSOR_OPTICAL:
                case messages::DeviceType::SENSOR_SONAR:
                    glyph_fn = state == messages::DeviceState::OK ? images::sensor_inactive : images::sensor_active;
                    break;
                case messages::DeviceType::BUTTON:
                case messages::DeviceType::SWITCH:
                case messages::DeviceType::MOTOR_WINCH:
                case messages::DeviceType::LIGHT:
                    glyph_fn = state == messages::DeviceState::OK ? images::winch_inactive : images::winch_active;
                    break;
            }
        }

        display.drawPngFromSd(glyph_fn.c_str(), x + 5, y + 5, false, inv);
        display.setTextColor(fg, bg);
        display.setTextWrap(false);

        display.setFont(&DejaVu_Sans_48);
        display.setCursor(x + 20 + images::glyph_size.width, y + 70);
        display.print(device.getDeviceName());

        display.setFont(&DejaVu_Sans_24);
        display.setCursor(x + 20 + images::glyph_size.width, y + 105);
        display.print(DosaDevice::stateToString(device.getDeviceState()));

        display.setFont(&Dialog_bold_16);
        printRight(
            DosaDevice::typeToString(device.getDeviceType()),
            x + images::panel_size.width - 25,
            y + images::panel_size.height - 55);
        printRight(
            comms::ipToString(device.getAddress().ip).c_str(),
            x + images::panel_size.width - 25,
            y + images::panel_size.height - 30);
    }

    /**
     * A pong message has been received.
     */
    void onPong(dosa::messages::Pong const& pong, comms::Node const& sender)
    {
        auto device = DosaDevice::fromPong(pong, sender);
        logln("Pong: " + device.getDeviceName() + " @ " + comms::ipToString(device.getAddress().ip), LogLevel::DEBUG);

        bool matched = false;

        for (auto& d : devices) {
            if (d.getAddress() == device.getAddress()) {
                // Device already registered
                matched = true;
                d.reportContact();
                if (d != device) {
                    // Device information has changed (name, health, state etc)
                    // NB: this will clear an error or no-contact device state
                    d = device;
                    setStatusMessage(d.getDeviceName() + " updated");
                }
                break;
            }
        }

        if (!matched) {
            if (devices.size() == devices.max_size()) {
                logln("Hit maximum device limit", LogLevel::ERROR);
                setDeviceState(messages::DeviceState::MINOR_FAULT);
                setStatusMessage("Max limit, cannot add " + device.getDeviceName());
            } else {
                logln("Adding device " + device.getDeviceName());
                devices.push_back(device);
                setStatusMessage("Discovered " + device.getDeviceName());
            }
        }
    }

    /**
     * A begin message has been received.
     */
    void onBegin(messages::GenericMessage const& msg, comms::Node const& sender)
    {
        for (auto& d : devices) {
            if (d.getAddress() == sender) {
                d.setDeviceState(messages::DeviceState::WORKING);
                d.reportContact();
                setStatusMessage(d.getDeviceName() + " start sequence");
                return;
            }
        }
    }

    /**
     * An end message has been received.
     */
    void onEnd(messages::GenericMessage const& msg, comms::Node const& sender)
    {
        for (auto& d : devices) {
            if (d.getAddress() == sender) {
                d.setDeviceState(messages::DeviceState::OK);
                d.reportContact();
                setStatusMessage(d.getDeviceName() + " end sequence");
                return;
            }
        }
    }

    /**
     * An online message has been received.
     *
     * This doesn't contain enough detail to create a new device item, but we can mark a device that was deemed
     * offline to now be back online.
     */
    void onOnline(messages::GenericMessage const& msg, comms::Node const& sender)
    {
        for (auto& d : devices) {
            if (d.getAddress() == sender) {
                d.setDeviceState(messages::DeviceState::OK);
                d.reportContact();
                setStatusMessage(d.getDeviceName() + " online");
                return;
            }
        }
    }

    /**
     * A trigger message has been received.
     */
    void onTrigger(messages::Trigger const& msg, comms::Node const& sender)
    {
        for (auto& d : devices) {
            if (d.getAddress() == sender) {
                d.setDeviceState(messages::DeviceState::TRIGGER);
                d.reportContact();
                last_ping = millis();  // prevent an update that could be moments away from hiding the sensor state
                setStatusMessage(d.getDeviceName() + " triggered");
                return;
            }
        }
    }

    /**
     * These functions forward a callback to a class-instance function.
     */
    static void pongMessageForwarder(messages::Pong const& pong, comms::Node const& sender, void* context)
    {
        static_cast<MonitorApp*>(context)->onPong(pong, sender);
    }

    static void onlineMessageForwarder(messages::GenericMessage const& msg, comms::Node const& sender, void* context)
    {
        static_cast<MonitorApp*>(context)->onOnline(msg, sender);
    }

    static void beginMessageForwarder(messages::GenericMessage const& msg, comms::Node const& sender, void* context)
    {
        static_cast<MonitorApp*>(context)->onBegin(msg, sender);
    }

    static void endMessageForwarder(messages::GenericMessage const& msg, comms::Node const& sender, void* context)
    {
        static_cast<MonitorApp*>(context)->onEnd(msg, sender);
    }

    static void trgMessageForwarder(messages::Trigger const& msg, comms::Node const& sender, void* context)
    {
        static_cast<MonitorApp*>(context)->onTrigger(msg, sender);
    }
};

}  // namespace dosa
