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
    Lights(Lights const&) = delete;
    void operator=(Lights const&) = delete;

    static Lights& getInstance()
    {
        static Lights instance;
        return instance;
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
        digitalWrite(LEDR, value ? LOW : HIGH);
    }

    /**
     * Enable/disable the green LED.
     */
    void setGreen(bool value)
    {
        digitalWrite(LEDG, value ? LOW : HIGH);
    }

    /**
     * Enable/disable the blue LED.
     */
    void setBlue(bool value)
    {
        digitalWrite(LEDB, value ? LOW : HIGH);
    }

    /**
     * Sets the state of all onboard lights.
     */
    void set(bool bin, bool red, bool green, bool blue)
    {
        setBuiltIn(bin);
        setRed(red);
        setGreen(green);
        setBlue(blue);
    }

   private:
    Lights()
    {
        pinMode(LEDR, OUTPUT);
        pinMode(LEDG, OUTPUT);
        pinMode(LEDB, OUTPUT);
        pinMode(LED_BUILTIN, OUTPUT);

        off();
    }
};

}  // namespace dosa