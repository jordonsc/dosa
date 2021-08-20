/**
 * Sensor unit.
 *
 * This programme should be burnt to satellite units that have PIR sensors and
 * will send a signal to the main unit to drive the winch.
 */

#include <Arduino.h>
#include <ArduinoBLE.h>
#include <dosa.h>

char const* sensorServiceId = "19b10000-e8f2-537e-4f6c-d104768a1214";
char const* sensorCharacteristicId = "19b10001-e8f2-537e-4f6c-d104768a1214";

BLEService sensorService(sensorServiceId);
BLEUnsignedShortCharacteristic sensorCharacteristic(sensorCharacteristicId, BLERead | BLENotify);

/**
 * Arduino setup
 */
void setup()
{
    auto& lights = dosa::Lights::getInstance();
    lights.setBuiltIn(true);

    auto& serial = dosa::SerialComms::getInstance();
    serial.setLogLevel(dosa::LogLevel::INFO);
    //serial.wait();

    serial.writeln("-- DOSA Motion Sensor --");
    serial.writeln("Begin init..");

    // Bluetooth init
    auto& bt = dosa::Bluetooth::getInstance();
    if (!bt.setEnabled(true) || !bt.setName("DOSA-S " + bt.localAddress().substring(15))) {
        serial.writeln("Bluetooth init failed", dosa::LogLevel::CRITICAL);
        errorHoldingPattern();
    }

    bt.setConnectionInterval(500, 3200);  // 0.5-4 seconds
    bt.setAppearance(0x0541);  // motion sensor

    if (!BLE.setAdvertisedService(sensorService)) {
        serial.writeln("Bluetooth failed to set advertised service", dosa::LogLevel::CRITICAL);
        errorHoldingPattern();
    }

    sensorService.addCharacteristic(sensorCharacteristic);
    BLE.addService(sensorService);
    sensorCharacteristic.writeValue(false);

    if (!bt.setAdvertise(true)) {
        serial.writeln("Bluetooth advertise failed", dosa::LogLevel::CRITICAL);
        errorHoldingPattern();
    }

    // For testing
    randomSeed(analogRead(0));

    // Init completed
    serial.writeln("Init complete\n");

    lights.off();
}

/**
 * Arduino main loop
 */
void loop()
{
    auto& serial = dosa::SerialComms::getInstance();
    auto& lights = dosa::Lights::getInstance();
    auto& bt = dosa::Bluetooth::getInstance();

    static String ctl = "";
    static unsigned long updated = millis();

    if (!bt.isEnabled()) {
        serial.writeln("BT not enabled!", dosa::LogLevel::CRITICAL);
        errorHoldingPattern();
    }

    BLEDevice central = BLE.central();

    // Calling connected() will also poll the BLE interface
    if (central && central.connected()) {
        if (ctl != central.address()) {
            serial.writeln("Connected to main driver: " + central.address());
            ctl = central.address();
            lights.set(false, false, false, true);
        }

        if (millis() - updated > 500) {
            sensorCharacteristic.writeValue(static_cast<unsigned short>(random(100, 200)));
            updated = millis();
        }
    } else {
        if (ctl != "") {
            serial.writeln("Disconnected from " + ctl);
            ctl = "";
            lights.off();
        }
    }
}
