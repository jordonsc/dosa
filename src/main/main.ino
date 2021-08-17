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
    Serial.begin(9600);

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
    delay(200);
    setLights(false, false, true, false);
    delay(200);
    setLights(false, false, false, true);
    delay(200);
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
    waitForSerial();

    if (!BLE.begin()) {
        Serial.println("ERROR: Starting BLE module failed!");
        errorPatternInit();
    }
}

/**
 * Sets the onboard lights.
 */
void setLights(bool bin, bool red, bool green, bool blue)
{
    digitalWrite(LED_BUILTIN, bin ? HIGH : LOW);

    digitalWrite(LEDR, red ? LOW : HIGH);
    digitalWrite(LEDG, green ? LOW : HIGH);
    digitalWrite(LEDB, blue ? LOW : HIGH);
}

/**
 * Waits for the serial interface to come up (user literally needs to open the serial monitor in the IDE).
 * 
 * Will blink a signal (yellow 500ms) while waiting.
 */
void waitForSerial() {
    while (!Serial) {
      setLights(true, true, true, false);
      delay(500);
      setLights(true, false, false, false);
      delay(500);
    };
}

/**
 * Loops indefinitely in an error pattern suggesting an 'init' error.
 */
void errorPatternInit()
{
  while (true) {
    setLights(true, true, false, false);
    delay(500);
    setLights(true, false, false, false);
    delay(500);
  };
}
