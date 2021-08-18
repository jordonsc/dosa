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
    auto lights = dosa::Lights::getInstance();
    lights.setBuiltIn(true);

    auto serial = dosa::SerialComms::getInstance();
    serial.wait();

    serial.writeln("-- DOSA Driver Unit --");
    serial.writeln("Begin init..");

    // Bluetooth init
    auto bt = dosa::Bluetooth::getInstance();
    bt.setName("DOSA Main Driver");
    bt.setAdvertise(true);

    // Init completed
    serial.writeln("Init complete\n");

    lights.off();
}

void connectToSensor(BLEDevice const& device)
{
    auto serial = dosa::SerialComms::getInstance();
    serial.writeln("Connecting to: " + device.localName() + " at " + device.address());
}

/**
 * Arduino main loop
 */
void loop()
{
    BLEDevice sensors[max_devices];

    auto lights = dosa::Lights::getInstance();
    auto bt = dosa::Bluetooth::getInstance();
    auto serial = dosa::SerialComms::getInstance();

    // Visual indicator of number of devices connected
    // TODO: this adds a lot of lag, replace this later
    for (const auto& sensor : sensors) {
        if (sensor && sensor.connected()) {
            lights.setBlue(true);
            delay(100);
            lights.setBlue(false);
            delay(100);
        }
    }
    delay(200);

    // Scan for new sensors to add
    auto sensor = bt.scanForService(sensorServiceId);
    if (sensor) {
        serial.writeln("Found sensor: " + sensor.address());

        // Check if we already know about this sensor
        bool registered = false;
        for (auto& i : sensors) {
            if (i) {
                if (i.address() == sensor.address()) {
                    registered = true;
                    // Already have this guy registered
                    if (!i.connected()) {
                        // No longer connected, reconnect
                        serial.writeln("- reconnecting..");
                        lights.setRed(true);
                        lights.setGreen(true);
                        connectToSensor(sensor);
                        delay(500);
                        lights.setRed(false);
                        lights.setGreen(false);
                    } else {
                        serial.writeln("- already connected");
                        lights.setRed(true);
                        lights.setBlue(true);
                        delay(500);
                        lights.setRed(false);
                        lights.setBlue(false);
                    }
                }
            }
        }

        if (!registered) {
            // Device was not in our registry, connect to it
            for (auto& i : sensors) {
                if (i) {
                    i = sensor;
                    lights.setGreen(true);
                    connectToSensor(sensor);
                    delay(500);
                    lights.setGreen(false);
                }
            }
        }
    }
}
