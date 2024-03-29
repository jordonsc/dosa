#pragma once

#include <ArduinoBLE.h>
#include <dosa_common.h>

#include <utility>

#include "config.h"
#include "const.h"
#include "container.h"
#include "message_pool.h"
#include "stats.h"

namespace dosa {

using NetLogLevel = messages::LogMessageLevel;

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
#define CONFIG_CHECK 50        // Time between checking if config values updated (ms)

/**
 * Wifi settings.
 *
 * While it makes sense to have a reasonably low WIFI_RECONNECT_WAIT time - this will also disconnect the BT which
 * will hide the device from someone trying to connect to update config.
 *
 * If a user connects via BT, we will NOT attempt to reconnect wifi until they disconnect.
 */
#define WIFI_CON_CHECK 500         // Time between checking health of wifi connection (ms)
#define WIFI_RECONNECT_WAIT 15000  // Time before reattempting to connect wifi (ms)
#define WIFI_INITIAL_ATTEMPTS 3    // Default number of attempts to connect wifi (first-run uses default)
#define WIFI_RETRY_ATTEMPTS 1      // Number of attempts to connect wifi after init

/**
 * Abstract App class that all devices should inherit.
 *
 * Contains support for FRAM, Bluetooth and Wifi - all bundled in this class. (Consider breaking it out?)
 */
class App : public virtual Loggable, public StatefulApplication
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
    {
    }

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
        setSerial(&(container.getSerial()));

        logln("-- " + config.app_name + " v" + String(DOSA_VERSION) + " --");
#ifdef DOSA_DEBUG
        logln("// Debug Mode //");
#endif
        logln("Begin init..");
        device_type = config.device_type;

        // Load settings from FRAM
        auto& settings = getSettings();
        if (!settings.load(false)) {
            // FRAM didn't contain valid settings, write default values to chip -
            settings.save(false);
        }
        logln("Device name: " + settings.getDeviceName());

        IPAddress addr;
        addr.fromString(getSettings().getStatsServerAddr());
        getStats().setStatsServer({addr, getSettings().getStatsServerPort()});
        getStats().setTags("app:" + settings.getDeviceName());

        // Bring BT online only if the wifi failed to connect
        if (settings.getWifiSsid().length() == 0 || !connectWifi()) {
            enableBluetooth();
        }

        // Handler for requests to drop back into Bluetooth mode for manual configuration
        getContainer().getComms().newHandler<comms::StandardHandler<messages::GenericMessage>>(
            DOSA_COMMS_MSG_BT_MODE,
            &btMessageForwarder,
            this);

        // Ping-Pong auto-responder
        getContainer().getComms().newHandler<comms::StandardHandler<messages::GenericMessage>>(
            DOSA_COMMS_MSG_PING,
            &pingMessageForwarder,
            this);

        // Debug auto-responder
        getContainer().getComms().newHandler<comms::StandardHandler<messages::GenericMessage>>(
            DOSA_COMMS_MSG_DEBUG,
            &debugMessageForwarder,
            this);

        // Config setting handler
        getContainer().getComms().newHandler<comms::StandardHandler<messages::Configuration>>(
            DOSA_COMMS_MSG_CONFIG,
            &configMessageForwarder,
            this);

        // Default flush handler
        getContainer().getComms().newHandler<comms::StandardHandler<messages::GenericMessage>>(
            DOSA_COMMS_MSG_FLUSH,
            &flushMessageForwarder,
            this);

        // Default stat handler
        getContainer().getComms().newHandler<comms::StandardHandler<messages::GenericMessage>>(
            DOSA_COMMS_MSG_REQ_STAT,
            &reqMessageForwarder,
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
            recordCommsStats();
        }
    };

    virtual void onWifiConnect()
    {
        if (getContainer().getComms().bindMulticast(comms::multicastAddr)) {
            logln("Listening for multicast packets", LogLevel::DEBUG);
            dispatchGenericMessage(DOSA_COMMS_MSG_ONLINE);
            getStats().count(stats::online);
        } else {
            logln("Failed to bind multicast", LogLevel::ERROR);
        }
    }

    virtual void onWifiDisconnect()
    {
        logln("Wifi connection lost", LogLevel::WARNING);
    }

    /**
     * Check if device is locked.
     */
    bool isLocked() const
    {
        return getSettings().getLockState() > LockState::UNLOCKED;
    }

    LockState getLockState() const
    {
        return getSettings().getLockState();
    }

   protected:
    MessagePool msg_cache;

    Settings& getSettings()
    {
        return getContainer().getSettings();
    }

    Settings const& getSettings() const
    {
        return getContainer().getSettings();
    }

    Stats& getStats()
    {
        return getContainer().getStats();
    }

    Stats const& getStats() const
    {
        return getContainer().getStats();
    }

    /**
     * Dispatch a generic message on the UDP multicast address.
     */
    bool dispatchGenericMessage(char const* cmd_code, bool wait_for_ack = false)
    {
        return getContainer().getComms().dispatch(
            comms::multicastAddr,
            messages::GenericMessage(cmd_code, getDeviceNameBytes()),
            wait_for_ack);
    }

    /**
     * Dispatch a specific message on the UDP multicast address.
     */
    bool dispatchMessage(messages::Payload const& payload, bool wait_for_ack = false)
    {
        return getContainer().getComms().dispatch(comms::multicastAddr, payload, wait_for_ack);
    }

    /**
     * Dispatch a network log message.
     */
    void netLog(char const* msg, NetLogLevel lvl = NetLogLevel::INFO)
    {
        cascadeNetLogMsg(msg, lvl);
        dispatchMessage(messages::LogMessage(msg, getDeviceNameBytes(), lvl), lvl >= NetLogLevel::WARNING);
    }

    void netLog(String const& msg, NetLogLevel lvl = NetLogLevel::INFO)
    {
        cascadeNetLogMsg(msg, lvl);
        dispatchMessage(messages::LogMessage(msg.c_str(), getDeviceNameBytes(), lvl), lvl >= NetLogLevel::WARNING);
    }

    /**
     * Dispatch a network log message to a specific destination
     */
    void netLog(char const* msg, comms::Node const& target, NetLogLevel lvl = NetLogLevel::INFO)
    {
        cascadeNetLogMsg(msg, lvl);
        getContainer().getComms().dispatch(
            target,
            messages::LogMessage(msg, getDeviceNameBytes(), lvl),
            lvl >= NetLogLevel::ERROR);
    }

    void netLog(String const& msg, comms::Node const& target, NetLogLevel lvl = NetLogLevel::INFO)
    {
        cascadeNetLogMsg(msg, lvl);
        getContainer().getComms().dispatch(
            target,
            messages::LogMessage(msg.c_str(), getDeviceNameBytes(), lvl),
            lvl >= NetLogLevel::ERROR);
    }

    void secAlert(SecurityLevel level)
    {
        dispatchMessage(messages::Security(level, getDeviceNameBytes()), true);
    }

    bool canTrigger(messages::Payload const& msg, comms::Node const& sender)
    {
        String sender_name = Comms::getDeviceName(msg);
        String sender_str = "'" + sender_name + "' (" + comms::nodeToString(sender) + ")";
        auto const& settings = getContainer().getSettings();

        if (!settings.isListenForAllDevices() && !settings.hasListenDevice(sender_name)) {
            logln("Ignoring trigger from " + sender_str, LogLevel::DEBUG);
            return false;
        } else {
            // Send reply ack, even if locked
            getContainer().getComms().dispatch(sender, messages::Ack(msg, getDeviceNameBytes()));

            if (isLocked()) {
                switch (getLockState()) {
                    case LockState::LOCKED:
                    default:
                        getStats().count(stats::sec_locked);
                        logln("Ignoring trigger while device is locked");
                        break;
                    case LockState::ALERT:
                        getStats().count(stats::sec_alert);
                        secAlert(SecurityLevel::ALERT);
                        netLog("Lock violation by: " + sender_str, NetLogLevel::WARNING);
                        break;
                    case LockState::BREACH:
                        getStats().count(stats::sec_breached);
                        secAlert(SecurityLevel::BREACH);
                        netLog("Security breach by: " + sender_str, NetLogLevel::WARNING);
                        break;
                }
                return false;

            } else {
                logln("Accepting trigger from " + sender_str);
            }
        }

        return true;
    }

    /**
     * Get a 20 byte array containing the local device name.
     *
     * This should be used for message creation.
     */
    char const* getDeviceNameBytes()
    {
        return getSettings().getDeviceNameBytes();
    }

    /**
     * Bring the Bluetooth chip online.
     */
    bool enableBluetooth()
    {
        auto& settings = getSettings();
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
        if (getSettings().getWifiSsid().length() == 0) {
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
                    wifi.disconnect();
                    wifi_connected = false;
                    onWifiDisconnect();
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
    [[nodiscard]] virtual Container const& getContainer() const = 0;

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
        auto& settings = getSettings();

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
        auto& settings = getSettings();

        if (getContainer().getBluetooth().isEnabled()) {
            getContainer().getBluetooth().setEnabled(false);
            central_connected = false;
            delay(NINA_CHIP_SWITCH_DELAY);
        }

        getContainer().getSerial().writeln("Connecting to wifi (" + settings.getWifiSsid() + ")..");
        if (getContainer().getWiFi().connect(settings.getWifiSsid(), settings.getWifiPassword(), attempts)) {
            wifi_connected = true;
            onWifiConnect();
            return true;
        } else {
            wifi_connected = false;
            return false;
        }
    }

    [[nodiscard]] bool isWifiConnected() const
    {
        return wifi_connected;
    }

    [[nodiscard]] bool isCentralConnected() const
    {
        return central_connected;
    }

    /**
     * DBG message received, automatically send log messages containing settings.
     *
     * You should override this (and still call this instance at the top of your overridden function).
     */
    virtual void onDebugRequest(messages::GenericMessage const& msg, comms::Node const& sender)
    {
        // NB: overriding functions are responsible for msg_cache validation!

        getStats().count("dosa.request.debug");

        auto const& settings = getSettings();
        logln("Debug request from '" + Comms::getDeviceName(msg) + "' (" + comms::nodeToString(sender) + ")");
        netLog("DOSA version: " + String(DOSA_VERSION), sender);
        switch (settings.getLockState()) {
            default:
                netLog("Device lock: unknown state", sender);
                break;
            case LockState::UNLOCKED:
                netLog("Device lock: unlocked", sender);
                break;
            case LockState::LOCKED:
                netLog("Device lock: LOCKED", sender);
                break;
            case LockState::ALERT:
                netLog("Device lock: ALERT", sender);
                break;
            case LockState::BREACH:
                netLog("Device lock: BREACH", sender);
                break;
        }
        netLog("Wifi AP: " + settings.getWifiSsid(), sender);

        if (settings.hasStatsServer()) {
            netLog("Stats server: " + settings.getStatsServerAddr() + ":" + settings.getStatsServerPort(), sender);
        } else {
            netLog("Stats server: none", sender);
        }

        auto const& listen_devices = settings.getListenDevices();
        if (listen_devices.length() == 0) {
            netLog("Listening to: <all devices>", sender);
        } else {
            int index, pos = 0;
            String listen_msg = "Listening to:";
            while ((index = listen_devices.indexOf('\n', pos)) != -1) {
                listen_msg += " '" + listen_devices.substring(pos, index) + "'";
                pos = index + 1;
            }
            netLog(listen_msg, sender);
        }
    }

    /**
     * FLS message received, reset device state.
     *
     * May be overridden.
     */
    virtual void onFlush(messages::GenericMessage const& msg, comms::Node const& sender)
    {
        getStats().count("dosa.request.flush");
        netLog("Flush request received from '" + Comms::getDeviceName(msg) + "' (" + comms::nodeToString(sender) + ")");
    }

    /**
     * REQ message received, return a stat message.
     *
     * May be overridden.
     */
    virtual void onRequestStat(messages::GenericMessage const& msg, comms::Node const& sender)
    {
        getStats().count("dosa.request.req-stat");
        netLog("Request-stat received from '" + Comms::getDeviceName(msg) + "' (" + comms::nodeToString(sender) + ")");

        // Send reply StatusMessage
        getContainer().getComms().dispatch(
            sender,
            messages::StatusMessage(
                static_cast<uint16_t>(messages::StatusFormat::STATUS_ONLY),
                String(static_cast<uint8_t>(getDeviceState())).c_str(),
                1,
                getSettings().getDeviceNameBytes()));
    }

   private:
    bool central_connected = false;
    bool wifi_connected = false;
    uint32_t config_last_checked = 0;
    uint32_t wifi_last_checked = 0;
    uint32_t wifi_last_reconnected = 0;
    messages::DeviceType device_type = messages::DeviceType::UNSPECIFIED;

    /**
     * Creates a serial log message for a NetLog message.
     */
    void cascadeNetLogMsg(String const& msg, NetLogLevel lvl)
    {
        switch (lvl) {
            case messages::LogMessageLevel::DEBUG:
                logln(msg, LogLevel::DEBUG);
                break;
            default:
            case messages::LogMessageLevel::INFO:
            case messages::LogMessageLevel::STATUS:
                logln(msg, LogLevel::INFO);
                break;
            case messages::LogMessageLevel::WARNING:
                logln(msg, LogLevel::WARNING);
                break;
            case messages::LogMessageLevel::ERROR:
                logln(msg, LogLevel::ERROR);
                break;
            case messages::LogMessageLevel::CRITICAL:
                logln(msg, LogLevel::CRITICAL);
                break;
        }
    }

    bool authCheck(String const& v)
    {
        if (v != getSettings().getPin()) {
            logln("BT auth error: '" + v + "', should be: '" + getSettings().getPin() + "'", dosa::LogLevel::ERROR);
            return false;
        } else {
            return true;
        }
    }

    /**
     * Checks the comms service for any stats it has ready and dispatches them (via stats via comms again) to the
     * statsd server.
     */
    void recordCommsStats()
    {
        auto& comms = getContainer().getComms();

        if (comms.getAckRetries() != -1) {
            getStats().gauge(stats::net_ack_retries, comms.getAckRetries());
        }

        if (comms.getAckTime() > 0) {
            getStats().timing(stats::net_ack_time, comms.getAckTime());
        }

        if (comms.getUnackedTriggers() > 0) {
            getStats().count(stats::net_unacked_triggers, comms.getUnackedTriggers());
        }

        comms.resetStats();
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
            auto& settings = getSettings();
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
        auto& settings = getSettings();

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
    void onBtModeRequest(messages::GenericMessage const& msg, comms::Node const& sender)
    {
        logln(
            "Received config-mode request message from '" + Comms::getDeviceName(msg) + "' (" +
            comms::nodeToString(sender) + ")");

        // Send reply ack
        getContainer().getComms().dispatch(sender, messages::Ack(msg, getDeviceNameBytes()));

        // Disconnect wifi and bring BT back online
        getSettings().setWifiSsid("");  // to prevent reconnects (don't write to FRAM!)
        getContainer().getWiFi().disconnect();
        wifi_connected = false;
        enableBluetooth();
    }

    /**
     * Ping message received, automatically send a pong.
     */
    void onPing(messages::GenericMessage const& msg, comms::Node const& sender)
    {
        // Reply to retries, but don't log them
        if (!msg_cache.validate(sender, msg.getMessageId())) {
            logln("Ping from '" + Comms::getDeviceName(msg) + "' (" + comms::nodeToString(sender) + ")");
        }

        // Send reply pong
        getContainer().getComms().dispatch(
            sender,
            messages::Pong(device_type, getDeviceState(), getSettings().getDeviceNameBytes()));
    }

    void settingPassword(String const& value)
    {
        auto& settings = getSettings();
        logln("SET PASSWORD: '" + value + "'");

        if (settings.setPin(value)) {
            settings.save();
        } else {
            logln("ERROR: failed to update device pin", LogLevel::ERROR);
        }
    }

    void settingDeviceName(String const& value)
    {
        auto& settings = getSettings();
        logln("SET DEVICE NAME: '" + value + "'");

        if (settings.setDeviceName(value)) {
            getStats().setTags("app:" + settings.getDeviceName());
            bt_device_name.writeValue(settings.getDeviceName());  // update BT value to new value
            settings.save();
        } else {
            logln("ERROR: failed to update device name", LogLevel::ERROR);
        }
    }

    void settingWifiAp(String const& value)
    {
        auto pos = value.indexOf("\n");
        if (pos == -1) {
            logln("ERROR: malformed wifi data", LogLevel::ERROR);
            return;
        } else if (pos == 0) {
            logln("CLEAR WIFI AP");

            auto& settings = getSettings();
            settings.setWifiSsid("");
            settings.setWifiPassword("");
            settings.save();

            // Wifi settings cleared, fallback into BT mode
            getContainer().getWiFi().disconnect();
            wifi_connected = false;
            enableBluetooth();

        } else {
            String ap = value.substring(0, pos);
            String pw = value.substring(pos + 1);

            logln("SET WIFI AP: '" + ap + "' / '" + pw + "'");

            auto& settings = getSettings();
            settings.setWifiSsid(ap);
            settings.setWifiPassword(pw);
            settings.save();

            // Connect to new wifi
            getContainer().getWiFi().disconnect();
            wifi_connected = false;
            connectWifiOrBluetooth(WIFI_RETRY_ATTEMPTS);
            wifi_last_reconnected = millis();
        }
    }

    void settingPirCalibration(uint8_t const* data, uint16_t size)
    {
        if (size != 9) {
            logln("ERROR: incorrect payload size for PIR calibration data", LogLevel::ERROR);
            return;
        }

        uint8_t min_pixels;
        float pixel_delta, total_delta;

        memcpy(&min_pixels, data, 1);
        memcpy(&pixel_delta, data + 1, 4);
        memcpy(&total_delta, data + 5, 4);

        logln("PIR CALIBRATION");
        logln(" > min pixels:  " + String(min_pixels));
        logln(" > pixel delta: " + String(pixel_delta));
        logln(" > total delta: " + String(total_delta));

        auto& settings = getSettings();
        settings.setPirMinPixels(min_pixels);
        settings.setPirPixelDelta(pixel_delta);
        settings.setPirTotalDelta(total_delta);
        settings.save();
    }

    void settingListenDevices(String const& value)
    {
        auto& settings = getSettings();

        String msg = value;
        msg.replace("\n", "; ");
        logln("SET LISTEN DEVICES: '" + msg + "'");

        settings.setListenDevices(value);
        settings.save();
    }

    void settingStatsServer(uint8_t const* data, uint16_t size)
    {
        auto& settings = getSettings();

        uint16_t server_port;
        memcpy(&server_port, data, 2);
        String server_addr = stringFromBytes(data + 2, size - 2);

        logln("SET STATS SERVER: " + server_addr + ":" + String(server_port));

        settings.setStatsServerAddr(server_addr);
        settings.setStatsServerPort(server_port);
        settings.save();

        IPAddress addr;
        addr.fromString(server_addr);
        getStats().setStatsServer({addr, server_port});
    }

    void settingDoorCalibration(uint8_t const* data, uint16_t size)
    {
        if (size != 14) {
            logln("ERROR: incorrect payload size for door calibration data", LogLevel::ERROR);
            return;
        }

        uint16_t open_distance;
        uint32_t open_wait, cool_down, close_ticks;

        memcpy(&open_distance, data, 2);
        memcpy(&open_wait, data + 2, 4);
        memcpy(&cool_down, data + 6, 4);
        memcpy(&close_ticks, data + 10, 4);

        logln("DOOR CALIBRATION");
        logln(" > open distance: " + String(open_distance));
        logln(" > open-wait:     " + String(open_wait));
        logln(" > cool-down:     " + String(cool_down));
        logln(" > close ticks:   " + String(close_ticks));

        auto& settings = getSettings();
        settings.setDoorOpenDistance(open_distance);
        settings.setDoorOpenWait(open_wait);
        settings.setDoorCoolDown(cool_down);
        settings.setDoorCloseTicks(close_ticks);
        settings.save();
    }

    void settingRangeCalibration(uint8_t const* data, uint16_t size)
    {
        if (size != 8) {
            logln("ERROR: incorrect payload size for range calibration data", LogLevel::ERROR);
            return;
        }

        uint16_t trigger_threshold, fixed_calibration;
        float trigger_coefficient;

        memcpy(&trigger_threshold, data, 2);
        memcpy(&fixed_calibration, data + 2, 2);
        memcpy(&trigger_coefficient, data + 4, 4);

        logln("RANGE CALIBRATION");
        logln(" > trigger threshold: " + String(trigger_threshold));
        logln(" > fixed calibration: " + String(fixed_calibration));
        logln(" > trigger coefficient: " + String(trigger_coefficient));

        auto& settings = getSettings();
        settings.setRangeTriggerThreshold(trigger_threshold);
        settings.setRangeFixedCalibration(fixed_calibration);
        settings.setRangeTriggerCoefficient(trigger_coefficient);
        settings.save();
    }

    void settingRelayCalibration(uint8_t const* data, uint16_t size)
    {
        if (size != 4) {
            logln("ERROR: incorrect payload size for relay calibration data", LogLevel::ERROR);
            return;
        }

        uint32_t relay_delay;
        memcpy(&relay_delay, data, 4);

        logln("RELAY CALIBRATION");
        logln(" > relay activation time: " + String(relay_delay));

        auto& settings = getSettings();
        settings.setRelayActivationTime(relay_delay);
        settings.save();
    }

    void settingDeviceLock(uint8_t const* data, uint16_t size)
    {
        if (size != 1) {
            logln("ERROR: incorrect payload size for lock configuration", LogLevel::ERROR);
            return;
        }

        LockState lock_state;
        memcpy(&lock_state, data, 1);

        logln("SET LOCK STATE");
        switch (lock_state) {
            default:
                logln(" > new state: UNKNOWN");
                break;
            case LockState::UNLOCKED:
                logln(" > new state: unlocked");
                break;
            case LockState::LOCKED:
                logln(" > new state: locked");
                break;
            case LockState::ALERT:
                logln(" > new state: alert");
                break;
            case LockState::BREACH:
                logln(" > new state: breach");
                break;
        }

        auto& settings = getSettings();
        settings.setLockState(lock_state);
        settings.save();
    }

    /**
     * Config setting packet received, update FRAM.
     */
    void onConfig(messages::Configuration const& msg, comms::Node const& sender)
    {
        // Send reply ack even for retries, but we won't double-set the config for duplicate message
        getContainer().getComms().dispatch(sender, messages::Ack(msg, getSettings().getDeviceNameBytes()));

        if (msg_cache.validate(sender, msg.getMessageId())) {
            return;
        }

        log("Config setting from '" + Comms::getDeviceName(msg) + "' (" + comms::nodeToString(sender) + ") // ");

        String metric("dosa.request.config");
        switch (msg.getConfigItem()) {
            case messages::Configuration::ConfigItem::PASSWORD:
                settingPassword(stringFromBytes(msg.getConfigData(), msg.getConfigSize()));
                metric += ".password";
                break;
            case messages::Configuration::ConfigItem::DEVICE_NAME:
                settingDeviceName(stringFromBytes(msg.getConfigData(), msg.getConfigSize()));
                metric += ".device_name";
                break;
            case messages::Configuration::ConfigItem::WIFI_AP:
                settingWifiAp(stringFromBytes(msg.getConfigData(), msg.getConfigSize()));
                metric += ".wifi_ap";
                break;
            case messages::Configuration::ConfigItem::PIR_CALIBRATION:
                settingPirCalibration(msg.getConfigData(), msg.getConfigSize());
                metric += ".pir";
                break;
            case messages::Configuration::ConfigItem::DOOR_CALIBRATION:
                settingDoorCalibration(msg.getConfigData(), msg.getConfigSize());
                metric += ".motor";
                break;
            case messages::Configuration::ConfigItem::RANGE_CALIBRATION:
                settingRangeCalibration(msg.getConfigData(), msg.getConfigSize());
                metric += ".range";
                break;
            case messages::Configuration::ConfigItem::RELAY_CALIBRATION:
                settingRelayCalibration(msg.getConfigData(), msg.getConfigSize());
                metric += ".relay";
                break;
            case messages::Configuration::ConfigItem::DEVICE_LOCK:
                settingDeviceLock(msg.getConfigData(), msg.getConfigSize());
                metric += ".lock";
                break;
            case messages::Configuration::ConfigItem::LISTEN_DEVICES:
                settingListenDevices(stringFromBytes(msg.getConfigData(), msg.getConfigSize()));
                metric += ".listen_devices";
                break;
            case messages::Configuration::ConfigItem::STATS_SERVER:
                settingStatsServer(msg.getConfigData(), msg.getConfigSize());
                metric += ".stats_server";
                break;
            default:
                logln("UNKNOWN SETTING");
                metric += ".unknown";
                break;
        }

        getStats().count(metric);
    }

    /**
     * Context forwarder for Bluetooth config-mode request messages.
     */
    static void btMessageForwarder(messages::GenericMessage const& msg, comms::Node const& sender, void* context)
    {
        static_cast<App*>(context)->onBtModeRequest(msg, sender);
    }

    /**
     * Context forwarder for ping request messages.
     */
    static void pingMessageForwarder(messages::GenericMessage const& msg, comms::Node const& sender, void* context)
    {
        static_cast<App*>(context)->onPing(msg, sender);
    }

    /**
     * Context forwarder for debug request messages.
     */
    static void debugMessageForwarder(messages::GenericMessage const& msg, comms::Node const& sender, void* context)
    {
        static_cast<App*>(context)->onDebugRequest(msg, sender);
    }

    /**
     * Context forwarder for config settings messages.
     */
    static void configMessageForwarder(messages::Configuration const& msg, comms::Node const& sender, void* context)
    {
        static_cast<App*>(context)->onConfig(msg, sender);
    }

    /**
     * Context forwarder for flush messages.
     */
    static void flushMessageForwarder(messages::GenericMessage const& msg, comms::Node const& sender, void* context)
    {
        static_cast<App*>(context)->onFlush(msg, sender);
    }

    /**
     * Context forwarder for req-stat messages.
     */
    static void reqMessageForwarder(messages::GenericMessage const& msg, comms::Node const& sender, void* context)
    {
        static_cast<App*>(context)->onRequestStat(msg, sender);
    }
};

}  // namespace dosa
