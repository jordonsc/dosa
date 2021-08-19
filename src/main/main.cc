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
    // serial.wait();

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
    serial.write("Connecting to " + device.address() + "..");
    if (device.connect()) {
        for (int i = 0; i < 3; ++i) {
            if (device.discoverAttributes()) {
                serial.writeln(" success");
                return true;
            } else {
                serial.write(".");

                if (!device.connected()) {
                    serial.writeln(" failed");
                    device.disconnect();
                    return false;
                }
            }
        }
        serial.writeln(" discovery failed");
        device.disconnect();
        return false;

    } else {
        serial.writeln(" failed");
        return false;
    }
}

/**
 * Arduino main loop
 */
void loop()
{
    static BLEDevice devices[max_devices];
    static BLECharacteristic sensors[max_devices];
    static unsigned short states[max_devices] = {0};

    auto& lights = dosa::Lights::getInstance();
    auto& bt = dosa::Bluetooth::getInstance();
    auto& serial = dosa::SerialComms::getInstance();

    if (!bt.isEnabled()) {
        serial.writeln("BT not enabled!", dosa::LogLevel::CRITICAL);
        errorHoldingPattern();
    }

    // Visual indicator of number of devices connected
    int connected = 0;
    for (unsigned i = 0; i < max_devices; ++i) {
        auto& d = devices[i];
        if (!d) {
            continue;
        }

        if (d.connected()) {
            unsigned short value = 0;
            sensors[i].readValue(value);
            if (value != states[i]) {
                serial.writeln("Sensor " + d.address() + " new state: " + value);
                states[i] = value;
            }

            ++connected;
        } else {
            serial.writeln("Failed: " + d.address());
            // d = BLEDevice();
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

    // Scan for new devices to add
    auto device = bt.scanForService(sensorServiceId);
    if (device) {
        // Check if we already know about this device
        for (auto& i : devices) {
            if (i && (i.address() == device.address())) {
                return;
            }
        }

        // Device was not in our registry, scan for a free slot and connect to it
        for (int i = 0; i < max_devices; ++i) {
            if (!devices[i]) {
                if (connectToSensor(device)) {
                    BLECharacteristic sensor = device.characteristic(sensorCharacteristicId);
                    if (!sensor) {
                        serial.writeln(
                            "Device " + device.address() + " is not reporting a sensor",
                            dosa::LogLevel::ERROR);
                        device.disconnect();
                    } else if (!sensor.canRead()) {
                        serial.writeln(
                            "Device" + device.address() + " is not granting sensor access",
                            dosa::LogLevel::ERROR);
                        device.disconnect();
                    } else {
                        devices[i] = device;
                        sensors[i] = sensor;
                    }
                }

                return;
            }
        }

        serial.writeln(
            "Unable to register sensor (" + device.address() + "): hit device limit",
            dosa::LogLevel::WARNING);
        lights.off();
        errorSignal();
        delay(1000);
    }
}
