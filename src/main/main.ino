/**
 * Main unit.
 *
 * This programme should be burnt to the main unit that is responsible for
 * driving the winch.
 */

#include <ArduinoBLE.h>
#include <dosa.h>

char const* deviceServiceUuid = "19b10000-e8f2-537e-4f6c-d104768a1214";
char const* deviceServiceCharacteristicUuid = "19b10001-e8f2-537e-4f6c-d104768a1214";
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
    serial.writeln("begin init..");

    // Bluetooth init
    auto bt = dosa::Bluetooth::getInstance();
    bt.setName("DOSA Main Driver");
    bt.setAdvertise(true);

    // Init completed
    serial.writeln("Init complete\n");

    lights.off();
    lights.setBlue(true);
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
    for (int i = 0; i < max_devices; ++i) {
        if (sensors[i] && sensors[i].connected()) {
            lights.setBlue(true);
            delay(100);
            lights.setBlue(false);
            delay(100);
        }
    }

    // Scan for new sensors to add
    auto sensor = bt.scanForService(deviceServiceUuid);
    if (sensor) {
        serial.writeln("Found sensor: " + sensor.address());

        // Check if we already know about this sensor
        bool registered = false;
        for (int i = 0; i < max_devices; ++i) {
            if (sensors[i]) {
                if (sensors[i].address() == sensor.address()) {
                    registered = true;
                    // Already have this guy registered
                    if (!sensors[i].connected()) {
                        // No longer connected, reconnect
                        serial.writeln("- reconnecting..");
                        lights.setRed(true);
                        lights.setGreen(true);
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
            for (int i = 0; i < max_devices; ++i) {
                if (sensors[i]) {
                    sensors[i] = sensor;
                    lights.setGreen(true);
                    delay(500);
                    lights.setGreen(false);
                }
            }
        }
    }
}
