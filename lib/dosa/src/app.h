#pragma once

#include <utility>

#include "config.h"
#include "const.h"
#include "container.h"

namespace dosa {

#define CENTRAL_CON_CHECK 500  // Time between checking health of central connection (ms)

class App
{
   public:
    explicit App(Config config) : config(std::move(config)), bt_char_version(dosa::bt::char_version, BLERead) {}
    explicit App(Config config, char const* bt_svc)
        : config(std::move(config)),
          bt_char_version(dosa::bt::char_version, BLERead),
          bt_service(bt_svc)
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
            bt.setConnectionInterval(DOSA_BT_DATA_MIN, DOSA_BT_DATA_MAX);
            if (config.bluetooth_appearance) {
                bt.setAppearance(config.bluetooth_appearance);
            }

            if (bt_service) {
                if (!BLE.setAdvertisedService(bt_service)) {
                    container.getSerial().writeln(
                        "Bluetooth failed to set advertised service",
                        dosa::LogLevel::CRITICAL);
                    container.getLights().errorHoldingPattern();
                }

                BLE.addService(bt_service);
            }

            // Add a version characteristic
            bt_service.addCharacteristic(bt_char_version);
            bt_char_version.writeValue(DOSA_VERSION);

            // Start advertising
            bt.setAdvertise(config.bluetooth_advertise);
        }

        // Init completed
        serial.writeln("Init complete\n");
        container.getLights().off();
    }

    virtual void loop() = 0;

   protected:
    /**
     * Check the state of the central device. Should be included in the main loop.
     */
    void checkCentral()
    {
        if (!getContainer().getBluetooth().isEnabled()) {
            getContainer().getSerial().writeln("BT not enabled!", dosa::LogLevel::CRITICAL);
            getContainer().getLights().errorHoldingPattern();
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

    virtual Container& getContainer() = 0;

    Config config;
    BLEUnsignedShortCharacteristic bt_char_version;

    BLEDevice bt_central;
    BLEService bt_service;

    bool central_connected = false;
    unsigned long central_last_health_check = 0;
};

}  // namespace dosa