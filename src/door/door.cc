/**
 * Door driver unit.
 *
 * This programme should be burnt to the door driver master device.
 */

#include <Arduino.h>
#include <ArduinoBLE.h>
#include <dosa.h>

#define PIN_SWITCH_DOOR 9
#define BLINK_INTERVAL 500

/**
 * Arduino setup
 */
void setup()
{
    auto& lights = dosa::Lights::getInstance();
    lights.setBuiltIn(true);

    // Serial
    auto& serial = dosa::SerialComms::getInstance();
    serial.setLogLevel(dosa::LogLevel::DEBUG);
    // serial.wait();

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

    // Set lights to ready configuration
    dosa::DoorLights door_lights;
    door_lights.ready();
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
    static dosa::Switch door_switch(PIN_SWITCH_DOOR, true);
    static dosa::Door door;
    static dosa::DoorLights door_lights;

    // Peripheral scan timer
    static unsigned long last_scan = 0;

    // When no devices are ready, we'll use this timer to blink the ready LED
    static unsigned long blink_timer = 0;
    static bool blink_state = true;

    // Check the door switch
    if (door_switch.process() && door_switch.getState()) {
        serial.writeln("Door switch pressed");
        door_lights.activity();
        door.trigger();
        door_lights.ready();
        blink_state = true;
    }

    // Process connected peripherals
    auto connected = pool.process();

    // Visual indicator of number of devices connected
    if (connected == 0) {
        if (millis() - blink_timer > BLINK_INTERVAL) {
            blink_timer = millis();
            blink_state = !blink_state;
            door_lights.setReady(blink_state);
        }
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
            door_lights.connecting();

            bt.stopScan();  // IMPORTANT: connection won't work if scanning
            pool.add(device);

            // Set notification devices to indicate we have a sensor ready
            door_lights.ready();
            blink_state = true;
        }
    }
}
