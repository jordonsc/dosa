#pragma once

#include <utility>

#include "config.h"
#include "const.h"
#include "container.h"

namespace dosa {

/**
 * Delay when switching between wifi/BT
 */
#define NINA_CHIP_SWITCH_DELAY 500

/**
 * Bluetooth settings.
 *
 * Bluetooth is used to update device settings.
 */
#define CENTRAL_CON_CHECK 500  // Time between checking health of central connection (ms)
#define CONFIG_CHECK 50  // Time between checking if config values updated (ms)

/**
 * Wifi settings.
 *
 * While it makes sense to have a reasonably low WIFI_RECONNECT_WAIT time - this will also disconnect the BT which
 * will hide the device from someone trying to connect to update config.
 *
 * If a user connects via BT, we will NOT attempt to reconnect wifi until they disconnect.
 */
#define WIFI_CON_CHECK 500  // Time between checking health of wifi connection (ms)
#define WIFI_RECONNECT_WAIT 60000  // Time before reattempting to connect wifi (ms)
#define WIFI_INITIAL_ATTEMPTS 5  // Default number of attempts to connect wifi (first-run uses default)
#define WIFI_RETRY_ATTEMPTS 5  // Number of attempts to connect wifi after init

/**
 * Abstract App class that all devices should inherit.
 *
 * Contains support for FRAM, Bluetooth and Wifi - all bundled in this class. (Consider breaking it out?)
 */
class App
{
   public:
    explicit App(Config config)
        : config(std::move(config)),
          bt_service(dosa::bt::svc_dosa),
          bt_version(dosa::bt::char_version, BLERead),
          bt_error_msg(dosa::bt::char_error_msg, BLERead | BLENotify, 255),
          bt_device_name(dosa::bt::char_device_name, BLERead | BLEWrite, 20),
          bt_set_pin(dosa::bt::char_set_pin, BLEWrite, 50),
          bt_set_wifi(dosa::bt::char_set_wifi, BLEWrite, 255)
    {}

    /**
     * Arduino init.
     *
     * Device start-up here.
     */
    virtual void init()
    {
        randomSeed(analogRead(0));

        // DI container
        auto& container = getContainer();

        // Lights
        container.getLights().setBuiltIn(true);

        // Serial
        container.getSerial().setLogLevel(config.log_level);
        if (config.wait_for_serial) {
            container.getSerial().wait();
        }

        logln("-- " + config.app_name + " --");
#ifdef DOSA_DEBUG
        logln("// Debug Mode //");
#endif
        logln("Begin init..");

        // Load settings from FRAM
        auto& settings = getContainer().getSettings();
        if (!settings.load()) {
            // FRAM didn't contain valid settings, write default values to chip -
            settings.save();
        }
        logln("Device name: " + settings.getDeviceName());

        // Bring BT online only if the wifi failed to connect
        if (settings.getWifiSsid().length() == 0 || !connectWifi()) {
            enableBluetooth();
        }

        // This is a wifi "config mode" request packet handler -
        getContainer().getComms().newHandler<comms::StandardHandler<messages::GenericMessage>>(
            DOSA_COMMS_CONFIG_MSG_CODE,
            &configMessageForwarder,
            this);

        wifi_last_checked = millis();
        wifi_last_reconnected = millis();

        // Init completed
        logln("Init complete\n");
        container.getLights().off();
    }

    /**
     * Main app loop.
     *
     * Common tasks included here, derived class should still execute this.
     */
    virtual void loop()
    {
        // Health-check on BT central device
        checkCentral();

        // Check if there have been any BT requests to change configuration
        checkConfigRequests();

        // Health-check (and reconnect attempt) for wifi (do not do this if we've got a BT device connected)
        if (!central_connected) {
            checkWifi();
        }

        // Check for inbound UDP messages
        if (wifi_connected) {
            getContainer().getComms().processInbound();
        }
    };

    virtual void onWifiConnect() {}

   protected:
    /**
     * Bring the Bluetooth chip online.
     */
    bool enableBluetooth()
    {
        auto& settings = getContainer().getSettings();
        auto& bt = getContainer().getBluetooth();

        static bool bt_service_init = false;

        central_connected = false;
        if (!bt.setEnabled(true) || !bt.setLocalName(settings.getDeviceName())) {
            logln("Bluetooth init failed", LogLevel::CRITICAL);
            bt.setEnabled(false);
            return false;
        }

        bt.setDeviceName(config.short_name + " " + bt.localAddress().substring(15));
        if (config.bluetooth_appearance) {
            bt.setAppearance(config.bluetooth_appearance);
        }

        // Add BT characteristics
        if (!bt_service_init) {
            bt_service.addCharacteristic(bt_version);
            bt_service.addCharacteristic(bt_error_msg);
            bt_service.addCharacteristic(bt_device_name);
            bt_service.addCharacteristic(bt_set_pin);
            bt_service.addCharacteristic(bt_set_wifi);

            bt_service_init = true;
        }

        bt_version.writeValue(DOSA_VERSION);
        bt_error_msg.writeValue("");
        bt_device_name.writeValue(settings.getDeviceName());
        bt_set_pin.writeValue("");
        bt_set_wifi.writeValue("");

        // Set BT service
        if (!bt.setService(bt_service)) {
            logln("Bluetooth failed to set advertised service", LogLevel::CRITICAL);
            bt.setEnabled(false);
            return false;
        }

        // Start advertising
        if (!bt.setAdvertise(true)) {
            logln("Failed to set BT to advertise mode", LogLevel::CRITICAL);
            bt.setEnabled(false);
            return false;
        }

        logln("Bluetooth online");
        return true;
    }

    /**
     * Check the state of the central device.
     *
     * Should be included in the main loop. This will toggle broadcasting when a device connects to us.
     * Called automatically by `stdLoop()`.
     */
    void checkCentral()
    {
        if (!getContainer().getBluetooth().isEnabled()) {
            return;
        }

        static unsigned long central_last_health_check = 0;

        if (central_connected) {
            if (millis() - central_last_health_check > CENTRAL_CON_CHECK) {
                central_last_health_check = millis();

                if (!bt_central.connected()) {
                    central_connected = false;
                    logln("Central disconnected");
                    getContainer().getBluetooth().setAdvertise(true);
                }
            }
        } else {
            bt_central = BLE.central();
            if (bt_central) {
                getContainer().getBluetooth().setAdvertise(false);
                central_last_health_check = millis();
                central_connected = true;
                logln("Connected to central: " + bt_central.address());
            }
        }
    }

    /**
     * Run a health-check on the wifi connection.
     *
     * Will periodically attempt to reconnect a broken wifi connection.
     */
    void checkWifi()
    {
        if (getContainer().getSettings().getWifiSsid().length() == 0) {
            return;
        }

        auto& wifi = getContainer().getWiFi();

        // We probe the wifi chip every WIFI_CON_CHECK ms..
        if (millis() - wifi_last_checked > WIFI_CON_CHECK) {
            wifi_last_checked = millis();

            if (!wifi_connected || !wifi.isConnected()) {
                // Wifi is offline
                if (wifi_connected) {
                    // it was online, mark is as a disconnect
                    logln("Wifi connection lost");
                    wifi.disconnect();
                    wifi_connected = false;
                }

                // Attempt to reconnect
                if (millis() - wifi_last_reconnected > WIFI_RECONNECT_WAIT) {
                    connectWifiOrBluetooth(WIFI_RETRY_ATTEMPTS);
                    wifi_last_reconnected = millis();
                }
            }
        }
    }

    /**
     * Checks for BT commands to update all standard config options.
     *
     * Has an internal timer to prevent hammering, safe to use in a loop.
     * Called automatically by `stdLoop()`.
     */
    void checkConfigRequests()
    {
        if (millis() - config_last_checked > CONFIG_CHECK && getContainer().getBluetooth().isEnabled()) {
            config_last_checked = millis();
            checkSetPin();
            checkSetWifi();
            checkSetDeviceName();
        }
    }

    [[nodiscard]] virtual Container& getContainer() = 0;

    Config config;

    BLEDevice bt_central;
    BLEService bt_service;

    BLEUnsignedShortCharacteristic bt_version;
    BLEStringCharacteristic bt_error_msg;
    BLEStringCharacteristic bt_device_name;
    BLEStringCharacteristic bt_set_pin;
    BLEStringCharacteristic bt_set_wifi;

    /**
     * Update wifi settings and write to FRAM.
     */
    void setWifi(String const& ssid, String const& password)
    {
        auto& settings = getContainer().getSettings();

        logln("Updating wifi AP to '" + ssid + "'");
        settings.setWifiSsid(ssid);
        settings.setWifiPassword(password);
        settings.save();
    }

    /**
     * Attempt to connect wifi, if it fails, enable Bluetooth.
     *
     * This is normally the way you should try to connect the wifi. If wifi fails, we need BLE enabled so that the
     * wifi can be reconfigured.
     */
    void connectWifiOrBluetooth(uint8_t attempts = WIFI_INITIAL_ATTEMPTS)
    {
        if (!connectWifi(attempts)) {
            delay(NINA_CHIP_SWITCH_DELAY);
            enableBluetooth();
        }
    }

    /**
     * Attempt to connect to the wifi AP.
     */
    bool connectWifi(uint8_t attempts = WIFI_INITIAL_ATTEMPTS)
    {
        auto& settings = getContainer().getSettings();

        if (getContainer().getBluetooth().isEnabled()) {
            getContainer().getBluetooth().setEnabled(false);
            central_connected = false;
            delay(NINA_CHIP_SWITCH_DELAY);
        }

        getContainer().getSerial().writeln("Connecting to wifi..");
        if (getContainer().getWiFi().connect(settings.getWifiSsid(), settings.getWifiPassword(), attempts)) {
            wifi_connected = true;
            onWifiConnect();
            return true;
        } else {
            wifi_connected = false;
            return false;
        }
    }

   private:
    bool central_connected = false;
    bool wifi_connected = false;
    unsigned long config_last_checked = 0;
    unsigned long wifi_last_checked = 0;
    unsigned long wifi_last_reconnected = 0;

    bool authCheck(String const& v)
    {
        if (v != getContainer().getSettings().getPin()) {
            logln(
                "BT auth error: '" + v + "', should be: '" + getContainer().getSettings().getPin() + "'",
                dosa::LogLevel::ERROR);
            return false;
        } else {
            return true;
        }
    }

    /**
     * Check for BT pin-set requests.
     */
    void checkSetPin()
    {
        auto v = bt_set_pin.value();
        if (v.length() == 0) {
            return;
        }

        bt_set_pin.writeValue("");  // to prevent repetition

        auto brk = v.indexOf('\n');  // delimiter between current pin and new pin
        if (brk < 1) {
            logln("Malformed pin-set request: '" + v + "'", dosa::LogLevel::ERROR);
            return;
        }

        if (!authCheck(v.substring(0, brk))) {
            return;
        }

        auto data_value = v.substring(brk + 1);

        if (data_value.length() >= 4 && data_value.length() < 50) {
            auto& settings = getContainer().getSettings();
            settings.setPin(data_value);
            settings.save();
            logln("Pin set to '" + data_value + "'");
        } else {
            logln("Pin must be between 4 and 50 chars: '" + data_value + "'", dosa::LogLevel::ERROR);
        }
    }

    /**
     * Check for BT device name-set requests.
     */
    void checkSetDeviceName()
    {
        auto& settings = getContainer().getSettings();

        auto v = bt_device_name.value();
        if (v == settings.getDeviceName()) {
            return;
        }

        // Set the device name back immediately to remove pin from value
        bt_device_name.writeValue(settings.getDeviceName());

        auto brk = v.indexOf('\n');
        if (brk < 1) {
            logln("Malformed device name-set request: '" + v + "'", dosa::LogLevel::ERROR);
            return;
        }

        if (!authCheck(v.substring(0, brk))) {
            return;
        }

        auto data_value = v.substring(brk + 1);

        if (data_value.length() >= 2 && data_value.length() <= 20) {
            if (settings.setDeviceName(data_value)) {
                logln("Device name set to '" + data_value + "'");
                settings.save();
                bt_device_name.writeValue(settings.getDeviceName());  // update BT value to new value
            } else {
                logln("Error updating device name: '" + data_value + "'", dosa::LogLevel::ERROR);
            }
        } else {
            logln("Device name must be between 2 and 20 chars: '" + data_value + "'", dosa::LogLevel::ERROR);
        }
    }

    /**
     * Check for BT wifi-set requests.
     */
    void checkSetWifi()
    {
        auto v = bt_set_wifi.value();
        if (v.length() == 0) {
            return;
        }

        bt_set_wifi.writeValue("");  // to prevent repetition
        auto& serial = getContainer().getSerial();

        auto brk = v.indexOf('\n');  // delimiter between current pin and new pin
        if (brk < 1) {
            logln("Malformed wifi-set request: '" + v + "'", dosa::LogLevel::ERROR);
            return;
        }

        if (!authCheck(v.substring(0, brk))) {
            return;
        }

        auto data_wifi = v.substring(brk + 1);
        brk = data_wifi.indexOf('\n');
        if (brk < 1) {
            logln("Malformed wifi-set request: '" + data_wifi + "'", dosa::LogLevel::ERROR);
            return;
        }

        setWifi(data_wifi.substring(0, brk), data_wifi.substring(brk + 1));

        if (data_wifi.substring(0, brk).length() > 0) {
            connectWifi();
        }
    }

    /**
     * UDP packet received requesting we disconnect wifi and bring BT back online for config changes.
     */
    void onConfigModeRequest(messages::GenericMessage const& msg, comms::Node const& sender)
    {
        logln(
            "Received config-mode request message from '" + Comms::getDeviceName(msg) + "' (" +
            comms::nodeToString(sender) + ")");

        // Send reply ack
        getContainer().getComms().dispatch(
            sender,
            messages::Ack(msg, getContainer().getSettings().getDeviceNameBytes()));

        // Disconnect wifi and bring BT back online
        getContainer().getSettings().setWifiSsid("");  // to prevent reconnects (don't write to FRAM!)
        getContainer().getWiFi().disconnect();
        wifi_connected = false;
        enableBluetooth();
    }

    /**
     * Context forwarder for config-mode request messages.
     */
    static void configMessageForwarder(messages::GenericMessage const& msg, comms::Node const& sender, void* context)
    {
        static_cast<App*>(context)->onConfigModeRequest(msg, sender);
    }

    void log(String const& s, dosa::LogLevel lvl = dosa::LogLevel::INFO)
    {
        getContainer().getSerial().write(s, lvl);
    }

    void logln(String const& s, dosa::LogLevel lvl = dosa::LogLevel::INFO)
    {
        getContainer().getSerial().writeln(s, lvl);
    }
};

}  // namespace dosa
