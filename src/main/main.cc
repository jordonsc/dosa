/**
 * Main unit.
 *
 * This programme should be burnt to the main unit that is responsible for
 * driving the winch.
 */

#include <Arduino.h>
#include <ArduinoBLE.h>
#include <dosa.h>

/**
 * Arduino setup
 */
void setup()
{
    auto& lights = dosa::Lights::getInstance();
    lights.setBuiltIn(true);

    auto& serial = dosa::SerialComms::getInstance();
    serial.setLogLevel(dosa::LogLevel::DEBUG);
    serial.wait();

    serial.writeln("-- DOSA Driver Unit --");
    serial.writeln("Begin init..");

    // Bluetooth init
    auto& bt = dosa::Bluetooth::getInstance();
    if (!bt.setEnabled(true) || !bt.setName("DOSA-D " + bt.localAddress().substring(15))) {
        serial.writeln("Bluetooth init failed", dosa::LogLevel::CRITICAL);
        errorHoldingPattern();
    }

    bt.setConnectionInterval(500, 3200);  // 0.5 - 4 seconds
    bt.setAppearance(0x0741);             // powered gate
    // bt.setAdvertise(true);

    // Init completed
    serial.writeln("Init complete\n");
    lights.off();
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

    static unsigned long last_scan = 0;

    if (!bt.isEnabled()) {
        serial.writeln("BT not enabled!", dosa::LogLevel::CRITICAL);
        errorHoldingPattern();
    }

    auto connected = pool.process();

    // Visual indicator of number of devices connected
    if (connected == 0) {
        lights.off();
    } else if (connected == 1) {
        lights.set(true, false, false, true);
    } else if (connected == 2) {
        lights.set(true, false, true, false);
    } else {
        lights.set(true, true, false, false);
    }

    // Scan for new devices to add
    if (millis() - last_scan > dosa::scan_freq) {
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
