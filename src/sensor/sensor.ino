/**
 * Sensor unit.
 *
 * This programme should be burnt to satellite units that have PIR sensors and
 * will send a signal to the main unit to drive the winch.
 */

#include <ArduinoBLE.h>

enum
{
    GESTURE_NONE = -1,
    GESTURE_UP = 0,
    GESTURE_DOWN = 1,
    GESTURE_LEFT = 2,
    GESTURE_RIGHT = 3
};

const char* deviceServiceUuid = "19b10000-e8f2-537e-4f6c-d104768a1214";
const char* deviceServiceCharacteristicUuid = "19b10001-e8f2-537e-4f6c-d104768a1214";

/**
 * Arduino setup
 */
void setup()
{
    // put your setup code here, to run once:
}

/**
 * Arduino main loop
 */
void loop()
{
    // put your main code here, to run repeatedly:
}
