/**
 * Sensor unit.
 *
 * This programme should be burnt to satellite units that have PIR sensors and will send a signal to the main unit to
 * drive the winch.
 */

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <Arduino.h>
#include <ArduinoBLE.h>
#include <dosa.h>

#define CON_CHECK 500  // Time between checking health of central connection (ms)
#define PIR_POLL 50    // How often we check the PIR sensor for state change
#define PIN_PIR 2      // Data pin for PIR sensor

BLEService sensor_service(dosa::bt::svc_service);
BLEByteCharacteristic sensor_characteristic(dosa::bt::char_sensor, BLERead | BLENotify);

/**
 * Arduino setup
 */
void setup()
{
    // DI container
    auto& container = dosa::Container::getInstance();

    // Lights
    container.getLights().setBuiltIn(true);

    // Serial
    auto& serial = container.getSerial();
    serial.setLogLevel(dosa::LogLevel::DEBUG);
    // serial.wait();

    serial.writeln("-- DOSA Motion Sensor --");
    serial.writeln("Begin init..");

    // Bluetooth init
    auto& bt = container.getBluetooth();
    if (!bt.setEnabled(true) || !bt.setLocalName("DOSA-S " + bt.localAddress().substring(15))) {
        serial.writeln("Bluetooth init failed", dosa::LogLevel::CRITICAL);
        container.getLights().errorHoldingPattern();
    }

    bt.setConnectionInterval(DOSA_BT_DATA_MIN, DOSA_BT_DATA_MAX);  // 0.5-4 seconds
    bt.setAppearance(0x0541);                                      // motion sensor

    if (!BLE.setAdvertisedService(sensor_service)) {
        serial.writeln("Bluetooth failed to set advertised service", dosa::LogLevel::CRITICAL);
        container.getLights().errorHoldingPattern();
    }

    sensor_service.addCharacteristic(sensor_characteristic);
    BLE.addService(sensor_service);
    sensor_characteristic.writeValue(1);

    if (!bt.setAdvertise(true)) {
        serial.writeln("Bluetooth advertise failed", dosa::LogLevel::CRITICAL);
        container.getLights().errorHoldingPattern();
    }

    // PIR init
    pinMode(PIN_PIR, INPUT);

    // Init completed
    serial.writeln("Init complete\n");
    container.getLights().off();
}

/**
 * Arduino main loop
 */
void loop()
{
    static auto& container = dosa::Container::getInstance();
    auto& serial = container.getSerial();
    auto& bt = container.getBluetooth();
    auto& pool = container.getDevicePool();
    auto& lights = container.getLights();

    static bool connected = false;
    static BLEDevice central;
    static unsigned long last_health_check = 0;
    static unsigned long last_updated = 0;

    static byte sensor_value = 1;

    if (!bt.isEnabled()) {
        serial.writeln("BT not enabled!", dosa::LogLevel::CRITICAL);
        lights.errorHoldingPattern();
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

    // Check state of the PIR sensor
    if (millis() - last_updated > PIR_POLL) {
        last_updated = millis();

        byte state = digitalRead(PIN_PIR) + 1;  // + 1 because we never want to send 0 as the value

        if (state != sensor_value) {
            sensor_value = state;
            serial.writeln("Update sensor value: " + String(sensor_value), dosa::LogLevel::DEBUG);
            sensor_characteristic.writeValue(sensor_value);
        }
    }
}

#pragma clang diagnostic pop