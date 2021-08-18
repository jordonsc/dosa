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
BLEByteCharacteristic sensorCharacteristic(sensorCharacteristicId, BLERead | BLEWrite);

int sensorState = -1;

/**
 * Arduino setup
 */
void setup()
{
    auto lights = dosa::Lights::getInstance();
    lights.setBuiltIn(true);

    auto serial = dosa::SerialComms::getInstance();
    serial.wait();

    serial.writeln("-- DOSA Motion Sensor --");
    serial.writeln("Begin init..");

    // Bluetooth init
    auto bt = dosa::Bluetooth::getInstance();
    bt.setName("DOSA Motion Sensor");

    BLE.setAdvertisedService(sensorService);
    sensorService.addCharacteristic(sensorCharacteristic);
    BLE.addService(sensorService);
    sensorCharacteristic.writeValue(sensorState);

    bt.setAdvertise(true);

    // Init completed
    serial.writeln("Init complete\n");

    lights.off();
}

/**
 * Arduino main loop
 */
void loop()
{
    auto serial = dosa::SerialComms::getInstance();

    BLEDevice central = BLE.central();
    waitSignal();

    if (central) {
        serial.writeln("Connected to main driver: " + central.address());
        successSignal();

        while (central.connected()) {
            if (sensorCharacteristic.written()) {
                sensorState = sensorCharacteristic.value();
            }
        }

        serial.writeln("Disconnected from main driver");
        errorSignal();
    }
}
