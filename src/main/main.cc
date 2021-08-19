/**
 * Main unit.
 *
 * This programme should be burnt to the main unit that is responsible for
 * driving the winch.
 */

#include <Arduino.h>
#include <ArduinoBLE.h>
#include <dosa.h>

char const* sensorServiceId = "19b10000-e8f2-537e-4f6c-d104768a1214";
char const* sensorCharacteristicId = "19b10001-e8f2-537e-4f6c-d104768a1214";
int const max_devices = 5;

/**
 * Arduino setup
 */
void setup()
{
    auto& lights = dosa::Lights::getInstance();
    lights.setBuiltIn(true);

    auto& serial = dosa::SerialComms::getInstance();
    serial.setLogLevel(dosa::LogLevel::INFO);
    serial.wait();

    serial.writeln("-- DOSA Driver Unit --");
    serial.writeln("Begin init..");

    // Bluetooth init
    auto& bt = dosa::Bluetooth::getInstance();
    if (!bt.setEnabled(true) || !bt.setName("DOSA-D " + bt.localAddress().substring(15))) {
        serial.writeln("Bluetooth init failed", dosa::LogLevel::CRITICAL);
        errorHoldingPattern();
    }

    bt.setAppearance(0x0741);  // powered gate
    bt.setAdvertise(true);

    // Init completed
    serial.writeln("Init complete\n");

    lights.off();
}

bool connectToSensor(BLEDevice& device)
{
    auto& serial = dosa::SerialComms::getInstance();
    serial.write("Connecting to " + device.address() + ".. ");
    if (device.connect()) {
        if (device.discoverAttributes()) {
            serial.writeln("success");
            return true;
        } else {
            serial.writeln("discovery failed");
            device.disconnect();
            return false;
        }
    } else {
        serial.writeln("failed");
        return false;
    }
}

/**
 * Arduino main loop
 */
void loop()
{
    static BLEDevice sensors[max_devices];

    auto& lights = dosa::Lights::getInstance();
    auto& bt = dosa::Bluetooth::getInstance();
    auto& serial = dosa::SerialComms::getInstance();

    if (!bt.isEnabled()) {
        serial.writeln("BT not enabled!", dosa::LogLevel::CRITICAL);
        errorHoldingPattern();
    }

    // Visual indicator of number of devices connected
    int connected = 0;
    for (auto& s : sensors) {
        if (!s) {
            continue;
        }

        if (s.connected()) {
            BLECharacteristic sensorCharacteristic = s.characteristic(sensorCharacteristicId);
            if (!sensorCharacteristic) {
                serial.writeln("Device " + s.address() + " is not reporting a sensor", dosa::LogLevel::ERROR);
                s.disconnect();
                continue;
            } else if (!sensorCharacteristic.canRead()) {
                serial.writeln("Device" + s.address() + " is not granting sensor access", dosa::LogLevel::ERROR);
                s.disconnect();
                continue;
            }

            sensorCharacteristic.read();
            unsigned short value = 0;
            sensorCharacteristic.readValue(value);
            serial.writeln("Sensor " + s.address() + " reporting state: " + (value == 0 ? "INACTIVE" : "ACTIVE"));

            ++connected;
        } else {
            serial.writeln("Disconnected: " + s.address());
            s = BLEDevice();
        }
    }

    if (connected == 0) {
        lights.off();
    } else if (connected == 1) {
        lights.blue();
    } else if (connected == 2) {
        lights.green();
    } else {
        lights.red();
    }

    // Scan for new sensors to add
    auto sensor = bt.scanForService(sensorServiceId);
    if (sensor) {
        // Check if we already know about this sensor
        for (auto& i : sensors) {
            if (i && (i.address() == sensor.address())) {
                return;
            }
        }

        // Device was not in our registry, scan for a free slot and connect to it
        for (auto& i : sensors) {
            if (!i) {
                if (connectToSensor(sensor)) {
                    i = sensor;
                }
                return;
            }
        }

        serial.writeln(
            "Unable to register sensor (" + sensor.address() + "): hit device limit",
            dosa::LogLevel::WARNING);
        lights.off();
        errorSignal();
        delay(1000);
    }
}
