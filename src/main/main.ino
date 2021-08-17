/**
 * Main unit.
 *
 * This programme should be burnt to the main unit that is responsible for
 * driving the winch.
 */

#include <ArduinoBLE.h>

const char* deviceServiceUuid = "19b10000-e8f2-537e-4f6c-d104768a1214";
const char* deviceServiceCharacteristicUuid = "19b10001-e8f2-537e-4f6c-d104768a1214";

/**
 * Arduino setup
 */
void setup()
{
    initLights();
    initComms();

    BLE.setLocalName("DOSA Driver Unit");
    BLE.advertise();

    Serial.println("DOSA Driver");
    Serial.println(" ");
}

/**
 * Arduino main loop
 */
void loop()
{
    setLights(false, true, false, false);
    delay(500);
    setLights(false, false, true, false);
    delay(500);
    setLights(false, false, false, true);
    delay(500);
}

void initLights()
{
    pinMode(LEDR, OUTPUT);
    pinMode(LEDG, OUTPUT);
    pinMode(LEDB, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);

    setLights(true, false, false, false);
}

void initComms()
{
    Serial.begin(9600);
    while (!Serial) {
    };

    if (!BLE.begin()) {
        Serial.println("ERROR: Starting BLE module failed!");
        setLights(true, true, false, false);
        while (1) {
        };
    }
}

void setLights(bool bin, bool red, bool green, bool blue)
{
    digitalWrite(LED_BUILTIN, bin ? HIGH : LOW);

    digitalWrite(LEDR, red ? HIGH : LOW);
    digitalWrite(LEDG, green ? HIGH : LOW);
    digitalWrite(LEDB, blue ? HIGH : LOW);
}
