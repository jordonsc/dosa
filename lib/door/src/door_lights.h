/**
 * Manages the lights on the door master unit.
 */

#pragma once

#include <ArduinoBLE.h>

#define PIN_LED_SWITCH 8
#define PIN_LED_READY 15
#define PIN_LED_ACTIVITY 16
#define PIN_LED_ERROR 17

namespace dosa::door {

class DoorLights
{
   public:
    DoorLights()
    {
        pinMode(PIN_LED_SWITCH, OUTPUT);
        pinMode(PIN_LED_READY, OUTPUT);
        pinMode(PIN_LED_ACTIVITY, OUTPUT);
        pinMode(PIN_LED_ERROR, OUTPUT);

        off();
    }

    /**
     * Turn all lights off.
     */
    void off()
    {
        set(false, false, false, false);
    }

    /**
     * Illuminate ready and door light only, all others off.
     */
    void ready()
    {
        set(true, true, false, false);
    }

    /**
     * Illuminate activity light only, all others off.
     */
    void activity()
    {
        set(false, false, true, false);
    }

    /**
     * Illuminate ready & activity light only, all others off.
     */
    void connecting()
    {
        set(false, true, true, false);
    }

    /**
     * Illuminate error light only, all others off.
     */
    void error()
    {
        set(false, false, false, true);
    }

    /**
     * Enable/disable the door switch ring LED.
     */
    void setSwitch(bool value)
    {
        digitalWrite(PIN_LED_SWITCH, value ? HIGH : LOW);
    }

    /**
     * Enable/disable the green 'ready' LED.
     */
    void setReady(bool value)
    {
        digitalWrite(PIN_LED_READY, value ? HIGH : LOW);
    }

    /**
     * Enable/disable the blue 'activity' LED.
     */
    void setActivity(bool value)
    {
        digitalWrite(PIN_LED_ACTIVITY, value ? HIGH : LOW);
    }

    /**
     * Enable/disable the red 'error' LED.
     */
    void setError(bool value)
    {
        digitalWrite(PIN_LED_ERROR, value ? HIGH : LOW);
    }

    /**
     * Sets the state of all door controller lights.
     */
    void set(bool door_switch, bool ready, bool activity, bool error)
    {
        setSwitch(door_switch);
        setReady(ready);
        setActivity(activity);
        setError(error);
    }
};

}  // namespace dosa::door
