#pragma once

#include <utility>

#include "config.h"
#include "const.h"
#include "container.h"

namespace dosa {

#define CENTRAL_CON_CHECK 500  // Time between checking health of central connection (ms)
#define CONFIG_CHECK 50  // Time between checking if config values updated (ms)

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

    virtual void init()
    {
        randomSeed(analogRead(0));

        // DI container
        auto& container = getContainer();

        // Lights
        container.getLights().setBuiltIn(true);

        // Serial
        auto& serial = container.getSerial();
        serial.setLogLevel(config.log_level);
        if (config.wait_for_serial) {
            serial.wait();
        }

        serial.writeln("-- " + config.app_name + " --");
#ifdef DOSA_DEBUG
        serial.writeln("// Debug Mode //");
#endif
        serial.writeln("Begin init..");

        // Load settings from FRAM
        auto& settings = getContainer().getSettings();
        if (!settings.load()) {
            // FRAM didn't contain valid settings, write default values to chip -
            settings.save();
        }
        getContainer().getSerial().writeln("Device name: " + settings.getDeviceName());

        if (settings.getWifiSsid().length() > 1 && wifiConnect()) {
            // Don't enable BT if wifi connected with saved settings
            config.bluetooth_enabled = false;
        }

        // Bluetooth init
        if (config.bluetooth_enabled) {
            auto& bt = container.getBluetooth();
            if (!bt.setEnabled(true) || !bt.setLocalName(settings.getDeviceName())) {
                serial.writeln("Bluetooth init failed", dosa::LogLevel::CRITICAL);
                container.getLights().errorHoldingPattern();
            }

            bt.setDeviceName(config.short_name + " " + bt.localAddress().substring(15));
            if (config.bluetooth_appearance) {
                bt.setAppearance(config.bluetooth_appearance);
            }

            // Add BT characteristics
            bt_service.addCharacteristic(bt_version);
            bt_version.writeValue(DOSA_VERSION);

            bt_service.addCharacteristic(bt_error_msg);
            bt_error_msg.writeValue("");

            bt_service.addCharacteristic(bt_device_name);
            bt_device_name.writeValue(settings.getDeviceName());

            bt_service.addCharacteristic(bt_set_pin);
            bt_set_pin.writeValue("");

            bt_service.addCharacteristic(bt_set_wifi);
            bt_set_wifi.writeValue("");

            // Add BT service
            if (!bt.setService(bt_service)) {
                serial.writeln("Bluetooth failed to set advertised service", dosa::LogLevel::CRITICAL);
                container.getLights().errorHoldingPattern();
            }

            // Start advertising
            bt.setAdvertise(config.bluetooth_advertise);
        }

        // Init completed
        serial.writeln("Init complete\n");
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

        // Check for inbound UDP messages
        getContainer().getComms().processInbound();
    };

    virtual void onWifiConnect() {}

   protected:
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

        if (central_connected) {
            if (millis() - central_last_health_check > CENTRAL_CON_CHECK) {
                central_last_health_check = millis();

                if (!bt_central.connected()) {
                    central_connected = false;
#ifdef DOSA_DEBUG
                    getContainer().getLights().off();
#endif
                    getContainer().getSerial().writeln("Central disconnected");
                    getContainer().getBluetooth().setAdvertise(true);
                }
            }
        } else {
            bt_central = BLE.central();
            if (bt_central) {
                getContainer().getBluetooth().setAdvertise(false);
                central_last_health_check = millis();
                central_connected = true;
#ifdef DOSA_DEBUG
                getContainer().getLights().set(false, false, false, true);
#endif
                getContainer().getSerial().writeln("Connected to central: " + bt_central.address());
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

    virtual Container& getContainer() = 0;

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

        getContainer().getSerial().writeln("Updating wifi AP to '" + ssid + "'");
        settings.setWifiSsid(ssid);
        settings.setWifiPassword(password);
        settings.save();
    }

    /**
     * Attempt to connect to the wifi AP.
     */
    bool wifiConnect()
    {
        auto& settings = getContainer().getSettings();
        getContainer().getSerial().writeln("Connecting to wifi..");

        if (getContainer().getBluetooth().isEnabled()) {
            getContainer().getBluetooth().setEnabled(false);
        }

        if (getContainer().getWiFi().connect(settings.getWifiSsid(), settings.getWifiPassword())) {
            onWifiConnect();
            return true;
        } else {
            return false;
        }
    }

   private:
    bool central_connected = false;
    unsigned long central_last_health_check = 0;
    unsigned long config_last_checked = 0;

    bool authCheck(String const& v)
    {
        if (v != getContainer().getSettings().getPin()) {
            getContainer().getSerial().writeln(
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
        auto& serial = getContainer().getSerial();

        auto brk = v.indexOf('\n');  // delimiter between current pin and new pin
        if (brk < 1) {
            serial.writeln("Malformed pin-set request: '" + v + "'", dosa::LogLevel::ERROR);
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
            serial.writeln("Pin set to '" + data_value + "'");
        } else {
            serial.writeln("Pin must be between 4 and 50 chars: '" + data_value + "'", dosa::LogLevel::ERROR);
        }
    }

    /**
     * Check for BT device name-set requests.
     */
    void checkSetDeviceName()
    {
        auto& serial = getContainer().getSerial();
        auto& settings = getContainer().getSettings();

        auto v = bt_device_name.value();
        if (v == settings.getDeviceName()) {
            return;
        }

        // Set the device name back immediately to remove pin from value
        bt_device_name.writeValue(settings.getDeviceName());

        auto brk = v.indexOf('\n');
        if (brk < 1) {
            serial.writeln("Malformed device name-set request: '" + v + "'", dosa::LogLevel::ERROR);
            return;
        }

        if (!authCheck(v.substring(0, brk))) {
            return;
        }

        auto data_value = v.substring(brk + 1);

        if (data_value.length() >= 2 && data_value.length() <= 20) {
            if (settings.setDeviceName(data_value)) {
                serial.writeln("Device name set to '" + data_value + "'");
                settings.save();
                bt_device_name.writeValue(settings.getDeviceName());  // update BT value to new value
            } else {
                serial.writeln("Error updating device name: '" + data_value + "'", dosa::LogLevel::ERROR);
            }
        } else {
            serial.writeln("Device name must be between 2 and 20 chars: '" + data_value + "'", dosa::LogLevel::ERROR);
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
            serial.writeln("Malformed wifi-set request: '" + v + "'", dosa::LogLevel::ERROR);
            return;
        }

        if (!authCheck(v.substring(0, brk))) {
            return;
        }

        auto data_wifi = v.substring(brk + 1);
        brk = data_wifi.indexOf('\n');
        if (brk < 1) {
            serial.writeln("Malformed wifi-set request: '" + data_wifi + "'", dosa::LogLevel::ERROR);
            return;
        }

        setWifi(data_wifi.substring(0, brk), data_wifi.substring(brk + 1));
        if (data_wifi.substring(0, brk).length() > 0) {
            wifiConnect();
        }
    }
};

}  // namespace dosa
