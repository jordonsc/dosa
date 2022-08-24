/**
 * Manages the lights on the door master unit.
 */

#pragma once

#include <ArduinoBLE.h>

#define PIN_LED_A 15
#define PIN_LED_B 16


namespace dosa {

class DoorLights
{
   public:
    DoorLights()
    {
        pinMode(PIN_LED_A, OUTPUT);
        pinMode(PIN_LED_B, OUTPUT);

        off();
    }

    /**
     * Turn all lights off.
     */
    void off()
    {
        set(false, false);
    }

    /**
     * Illuminate ready and door light only, all others off.
     */
    void ready()
    {
        set(true, false);
    }

    /**
     * Illuminate activity light only, all others off.
     */
    void activity()
    {
        set(true, true);
    }

    /**
     * Illuminate ready & activity light only, all others off.
     */
    void connecting()
    {
        set(false, false);
    }

    /**
     * Illuminate error light only, all others off.
     */
    void error()
    {
        set(false, true);
    }

    /**
     * Enable/disable the door switch ring LED.
     */
    void setLedA(bool value)
    {
        digitalWrite(PIN_LED_A, value ? HIGH : LOW);
    }

    /**
     * Enable/disable the green 'ready' LED.
     */
    void setLedB(bool value)
    {
        digitalWrite(PIN_LED_B, value ? HIGH : LOW);
    }


    /**
     * Sets the state of all door controller lights.
     */
    void set(bool led_a, bool led_b)
    {
        setLedA(led_a);
        setLedB(led_b);
    }

    void sequenceWarning()
    {
        for (int i = 0; i < 3; ++i) {
            set(true, false);
            delay(150);
            set(false, true);
            delay(150);
        }
        off();
    }

    void sequenceError()
    {
        for (int i = 0; i < 3; ++i) {
            set(true, true);
            delay(350);
            set(false, false);
            delay(250);
        }
        off();
    }
};

}  // namespace dosa
