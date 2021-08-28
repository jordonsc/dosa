/**
 * Door driver unit.
 *
 * This programme should be burnt to the door driver master device.
 */

#include <Arduino.h>
#include <ArduinoBLE.h>
#include <dosa.h>

#define PIN_SWITCH 9
#define PIN_SWITCH_LED 10

/**
 * Arduino setup
 */
void setup()
{
    auto& lights = dosa::Lights::getInstance();
    lights.setBuiltIn(true);

    // Switch
    pinMode(PIN_SWITCH_LED, OUTPUT);

    // Serial
    auto& serial = dosa::SerialComms::getInstance();
    serial.setLogLevel(dosa::LogLevel::DEBUG);
    serial.wait();

    serial.writeln("-- DOSA Driver Unit --");
    serial.writeln("Begin init..");

    // Bluetooth init
    auto& bt = dosa::Bluetooth::getInstance();
    if (!bt.setEnabled(true) || !bt.setLocalName("DOSA-D " + bt.localAddress().substring(15))) {
        serial.writeln("Bluetooth init failed", dosa::LogLevel::CRITICAL);
        errorHoldingPattern();
    }

    bt.setConnectionInterval(DOSA_BT_DATA_MIN, DOSA_BT_DATA_MAX);  // 0.5 - 4 seconds
    bt.setAppearance(0x0741);                                      // powered gate
    // bt.setAdvertise(true);   // Driver should only advertise for debug purposes

    // Init completed
    serial.writeln("Init complete\n");
    lights.off();
    digitalWrite(PIN_SWITCH_LED, HIGH);

    // Motor test
    dosa::Door door;
    door.motorTest();
}

/**
 * Arduino main loop
 */
void loop()
{
    static auto& lights = dosa::Lights::getInstance();
    static auto& bt = dosa::Bluetooth::getInstance();
    static auto& serial = dosa::SerialComms::getInstance();
    static auto& pool = dosa::DevicePool::getInstance();
    static dosa::Switch door_switch(PIN_SWITCH);

    static unsigned long last_scan = 0;

    if (!bt.isEnabled()) {
        serial.writeln("BT not enabled!", dosa::LogLevel::CRITICAL);
        errorHoldingPattern();
    }

    // Check the door switch
    door_switch.process();

    // Process connected peripherals
    auto connected = pool.process();

    // Visual indicator of number of devices connected
    if (connected == 0) {
        lights.off();
    } else {
        lights.setBuiltIn(true);
    }

    // Scan for new devices to add
    if (millis() - last_scan > DOSA_SCAN_FREQ) {
        last_scan = millis();

        if (!bt.isScanning()) {
            // Not in scan mode, start the scanner and wait for next run
            bt.scanForService(dosa::sensor_svc_id);
            return;
        }

        serial.writeln("(" + String(connected) + ") Scanning..", dosa::LogLevel::DEBUG);

        auto device = BLE.available();
        if (device) {
            bt.stopScan();  // IMPORTANT: connection won't work if scanning
            pool.add(device);
        }
    }
}
