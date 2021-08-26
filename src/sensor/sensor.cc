/**
 * Sensor unit.
 *
 * This programme should be burnt to satellite units that have PIR sensors and
 * will send a signal to the main unit to drive the winch.
 */

#include <Arduino.h>
#include <ArduinoBLE.h>
#include <dosa.h>

// Time between checking health of central connection (ms)
#define CON_CHECK 500

BLEService sensorService(dosa::sensor_svc_id);
BLEUnsignedShortCharacteristic sensorCharacteristic(dosa::sensor_char_id, BLERead | BLENotify);

/**
 * Arduino setup
 */
void setup()
{
    auto& lights = dosa::Lights::getInstance();
    lights.setBuiltIn(true);

    auto& serial = dosa::SerialComms::getInstance();
    serial.setLogLevel(dosa::LogLevel::DEBUG);
    // serial.wait();

    serial.writeln("-- DOSA Motion Sensor --");
    serial.writeln("Begin init..");

    // Bluetooth init
    auto& bt = dosa::Bluetooth::getInstance();
    if (!bt.setEnabled(true) || !bt.setName("DOSA-S " + bt.localAddress().substring(15))) {
        serial.writeln("Bluetooth init failed", dosa::LogLevel::CRITICAL);
        errorHoldingPattern();
    }

    bt.setConnectionInterval(500, 3200);  // 0.5-4 seconds
    bt.setAppearance(0x0541);             // motion sensor

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
    static auto& serial = dosa::SerialComms::getInstance();
    static auto& lights = dosa::Lights::getInstance();
    static auto& bt = dosa::Bluetooth::getInstance();

    static bool connected = false;
    static BLEDevice central;
    static unsigned long last_health_check = 0;
    static unsigned long last_updated = 0;

    static unsigned short sensor_value = 1;

    if (!bt.isEnabled()) {
        serial.writeln("BT not enabled!", dosa::LogLevel::CRITICAL);
        errorHoldingPattern();
    }

    if (connected) {
        if (millis() - last_health_check > CON_CHECK) {
            last_health_check = millis();

            if (!central.connected()) {
                connected = false;
                lights.off();
                serial.writeln("Disconnected");
                bt.setAdvertise(true);
            }
        }
    } else {
        central = BLE.central();
        if (central) {
            bt.setAdvertise(false);
            last_health_check = millis();
            connected = true;
            lights.set(true, false, false, true);
            serial.writeln("Connected to main driver: " + central.address());
        }
    }

    // Test data - change random value every 2 seconds
    if (millis() - last_updated > 2000) {
        last_updated = millis();
        sensor_value = static_cast<unsigned short>(random(100, 200));
        serial.writeln("Update sensor value: " + String(sensor_value));
        sensorCharacteristic.writeValue(sensor_value);
    }
}
