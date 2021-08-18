/**
 * Serial communications (USB).
 */

#pragma once

#include <Arduino.h>

#include "dosa_error.h"
#include "dosa_lights.h"

#ifndef SERIAL_BUFFER_SIZE
class Serial_
{
   public:
    void begin(unsigned long) {}
    void print(String const&) {}
    void println(String const&) {}

    explicit operator bool()
    {
        return true;
    }
};

Serial_ Serial;
#endif

namespace dosa {

class SerialComms
{
   public:
    static SerialComms& getInstance()
    {
        static SerialComms instance;
        return instance;
    }

    /**
     * Write to serial interface.
     */
    void write(String const& msg)
    {
        Serial.print(msg);
    }

    /**
     * Write to serial interface, include a trailing new-line.
     */
    void writeln(String const& msg)
    {
        Serial.println(msg);
    }

    /**
     * Waits for the serial interface to come up (user literally needs to open the serial monitor in the IDE).
     *
     * Will blink a signal (yellow 500ms) while waiting.
     */
    void wait()
    {
        auto lights = dosa::Lights::getInstance();

        while (!Serial) {
            lights.setRed(true);
            lights.setGreen(true);
            delay(500);
            lights.setRed(false);
            lights.setGreen(false);
            delay(500);
        }
    }

   private:
    SerialComms()
    {
        Serial.begin(9600);
    }
};

}  // namespace dosa
