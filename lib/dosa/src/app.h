#pragma once

#include <utility>

#include "config.h"
#include "const.h"
#include "container.h"

namespace dosa {

#define CENTRAL_CON_CHECK 500  // Time between checking health of central connection (ms)
#define CONFIG_CHECK 50  // Time between checking if config values updated (ms)
#define WIFI_CHECK 500  // Wifi polling period (ms)

class App
{
   public:
    explicit App(Config config)
        : config(std::move(config)),
          bt_service(dosa::bt::svc_dosa),
          bt_version(dosa::bt::char_version, BLERead),
          bt_error_msg(dosa::bt::char_error_msg, BLERead | BLENotify, 255),
          bt_set_pin(dosa::bt::char_set_pin, BLEWrite, 50),
          bt_set_wifi(dosa::bt::char_set_wifi, BLEWrite, 255)
    {}

    virtual void init()
    {
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

        // Bluetooth init
        if (config.bluetooth_enabled) {
            auto& bt = container.getBluetooth();
            if (!bt.setEnabled(true) || !bt.setLocalName(config.short_name + " " + bt.localAddress().substring(15))) {
                serial.writeln("Bluetooth init failed", dosa::LogLevel::CRITICAL);
                container.getLights().errorHoldingPattern();
            }

            bt.setDeviceName(config.short_name);
            // bt.setConnectionInterval(DOSA_BT_DATA_MIN, DOSA_BT_DATA_MAX);
            if (config.bluetooth_appearance) {
                bt.setAppearance(config.bluetooth_appearance);
            }

            // Add BT characteristics
            bt_service.addCharacteristic(bt_version);
            bt_version.writeValue(DOSA_VERSION);

            bt_service.addCharacteristic(bt_error_msg);
            bt_error_msg.writeValue("");

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

    virtual void loop() = 0;
    virtual void onWifiConnect() {}

   protected:
    /**
     * Runs all common loop tasks.
     *
     * Checks the BT central & all configuration updates.
     */
    void stdLoop()
    {
        checkCentral();
        checkConfigRequests();
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
        if (millis() - config_last_checked > CONFIG_CHECK) {
            config_last_checked = millis();
            checkSetPin();
            checkSetWifi();
        }
    }

    virtual Container& getContainer() = 0;

    Config config;

    BLEDevice bt_central;
    BLEService bt_service;

    BLEUnsignedShortCharacteristic bt_version;
    BLEStringCharacteristic bt_error_msg;
    BLEStringCharacteristic bt_set_pin;
    BLEStringCharacteristic bt_set_wifi;

    void setWifi(String const& ssid, String const& password)
    {
        auto& serial = getContainer().getSerial();

        wifi_ssid = ssid;
        wifi_password = password;

        getContainer().getBluetooth().setEnabled(false);
        if (getContainer().getWiFi().connect(wifi_ssid, wifi_password)) {
            onWifiConnect();
        } else {
            getContainer().getBluetooth().setEnabled(true);
        }
    }

   private:
    bool central_connected = false;
    unsigned long central_last_health_check = 0;
    unsigned long config_last_checked = 0;
    String pin = "dosa";
    String wifi_ssid;
    String wifi_password;

    bool authCheck(String const& v)
    {
        if (v != pin) {
            getContainer().getSerial().writeln("BT auth error: '" + v + "'", dosa::LogLevel::ERROR);
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

        if (data_value.length() >= 4) {
            pin = data_value;
            serial.writeln("Set pin to '" + pin + "'");
        } else {
            serial.writeln("New pin too short: '" + data_value + "'", dosa::LogLevel::ERROR);
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
    }
};

}  // namespace dosa
