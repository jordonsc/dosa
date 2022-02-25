#pragma once

#include <Vector.h>
#include <comms.h>
#include <dosa_inkplate.h>

#include "const.h"
#include "dosa_device.h"

#define DOSA_PING_INTERVAL 10000

namespace dosa {

class MonitorApp : public InkplateApp
{
   public:
    explicit MonitorApp(InkplateConfig const& cfg) : InkplateApp(cfg, INKPLATE_1BIT, &serial) {}

    void init() override
    {
        InkplateApp::init();

        // Clear the splash and display the main screen
        printMain();

        // Bind a pong handler and send initial ping while we wait for the screen to do a full refresh
        getComms().newHandler<comms::StandardHandler<messages::Pong>>(DOSA_COMMS_MSG_PONG, &pongMessageForwarder, this);
        sendPing();

        // This will be >= 5 seconds from the splash screen first appearing
        fullRefreshWhenReady();
        logln("Monitor online");
    }

    void loop() override
    {
        if (millis() - last_ping > DOSA_PING_INTERVAL) {
            checkWifi();
            sendPing();
        }

        /**
         * Button 1: send trigger
         */
        if (getDisplay().readTouchpad(PAD1)) {
            sendTrigger();
        }

        /**
         * Button 2: clear device list and refresh
         */
        if (getDisplay().readTouchpad(PAD2)) {
            devices.clear();
            refreshDisplay(true);
            sendPing();
        }

        /**
         * Button 3: full-screen refresh
         */
        if (getDisplay().readTouchpad(PAD3)) {
            refreshDisplay(true);
        }

        delay(100);
    }

   private:
    SerialComms serial;
    Vector<DosaDevice> devices;
    uint32_t last_ping = 0;

    /**
     * Checks and reconnects the wifi if it disconnected.
     *
     * May never return if wifi is unavailable. Returns true if this function needed to reconnect.
     * @return
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

            printMain();
            fullRefreshWhenReady();
            return true;
        } else {
            return false;
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
        logln("Ping");
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
        getDisplay().clearDisplay();
        for (uint8_t i = 0; i < (uint8_t)devices.size(); ++i) {
            printDevice(i, devices[i]);
        }
    }

    /**
     * Print a device info panel.
     */
    void printDevice(uint8_t pos, DosaDevice const& device)
    {
        auto state = device.getDeviceState();
        int x = 10;
        int y = (pos * (dosa::images::panel_size.height + 5)) + 10;  // 5 spacing, 5 top margin

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
     * Print a device panel, part of the main display.
     */
    void printPanel(uint8_t pos, char const* img, String const& title, String const& status, bool inv = false) {}

    /**
     * A pong message has been received.
     */
    void onPong(dosa::messages::Pong const& pong, comms::Node const& sender)
    {
        auto device = DosaDevice::fromPong(pong, sender);
        logln("Pong: " + device.getDeviceName() + " @ " + comms::ipToString(device.getAddress().ip));

        bool matched = false;
        bool changed = false;

        for (auto& d : devices) {
            if (d.getAddress() == device.getAddress()) {
                // Device already registered
                matched = true;
                if (d != device) {
                    // Device information has changed (name, health, etc)
                    d = device;
                    changed = true;
                }
                break;
            }
        }

        if (!matched) {
            devices.push_back(device);
            changed = true;
        }

        if (changed) {
            printMain();
            refreshDisplay();
        }
    }

    /**
     * Context forwarder for pong messages.
     */
    static void pongMessageForwarder(dosa::messages::Pong const& pong, comms::Node const& sender, void* context)
    {
        static_cast<MonitorApp*>(context)->onPong(pong, sender);
    }
};

}  // namespace dosa
