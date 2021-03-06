/**
 * Manages the built-in LEDs.
 */

#pragma once

#include <Arduino.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 0
#endif

#ifndef LEDR
#define LEDR 0
#endif

#ifndef LEDG
#define LEDG 0
#endif

#ifndef LEDB
#define LEDB 0
#endif

namespace dosa {

class Lights
{
   public:
    Lights()
    {
#if LEDR > 0
        pinMode(LEDR, OUTPUT);
#endif
#if LEDG > 0
        pinMode(LEDG, OUTPUT);
#endif
#if LEDB > 0
        pinMode(LEDB, OUTPUT);
#endif
        pinMode(LED_BUILTIN, OUTPUT);

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
     * Illuminate red light only, all others off.
     */
    void red()
    {
        set(false, true, false, false);
    }

    /**
     * Illuminate green light only, all others off.
     */
    void green()
    {
        set(false, false, true, false);
    }

    /**
     * Illuminate blue light only, all others off.
     */
    void blue()
    {
        set(false, false, false, true);
    }

    /**
     * Enable/disable the "built-in" amber LED.
     */
    void setBuiltIn(bool value)
    {
        digitalWrite(LED_BUILTIN, value ? HIGH : LOW);
    }

    /**
     * Enable/disable the red LED.
     */
    void setRed(bool value)
    {
#if LEDR > 0
        digitalWrite(LEDR, value ? LOW : HIGH);
#endif
    }

    /**
     * Enable/disable the green LED.
     */
    void setGreen(bool value)
    {
#if LEDG > 0
        digitalWrite(LEDG, value ? LOW : HIGH);
#endif
    }

    /**
     * Enable/disable the blue LED.
     */
    void setBlue(bool value)
    {
#if LEDB > 0
        digitalWrite(LEDB, value ? LOW : HIGH);
#endif
    }

    /**
     * Sets the state of all onboard lights.
     */
    void set(bool bin, bool red, bool green, bool blue)
    {
        setBuiltIn(bin);
#if LEDR > 0
        setRed(red);
#endif
#if LEDG > 0
        setGreen(green);
#endif
#if LEDB > 0
        setBlue(blue);
#endif
    }

    [[noreturn]] void errorHoldingPattern()
    {
        while (true) {
            set(true, true, false, false);
            delay(1000);
            off();
            delay(1000);
        }
    }
};

}  // namespace dosa
