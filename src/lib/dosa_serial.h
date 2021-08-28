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

enum class LogLevel
{
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARNING = 3,
    ERROR = 4,
    CRITICAL = 5
};

class SerialComms
{
   public:
    SerialComms(SerialComms const&) = delete;
    void operator=(SerialComms const&) = delete;

    static SerialComms& getInstance()
    {
        static SerialComms instance;
        return instance;
    }

    void setLogLevel(LogLevel level)
    {
        loggingLevel = level;
    }

    /**
     * Write to serial interface.
     */
    void write(String const& msg, LogLevel level = LogLevel::INFO)
    {
        if (level >= loggingLevel) {
            Serial.print(msg);
        }
    }

    /**
     * Write to serial interface, include a trailing new-line.
     */
    void writeln(String const& msg, LogLevel level = LogLevel::INFO)
    {
        if (level >= loggingLevel) {
            Serial.println(msg);
        }
    }

    /**
     * Waits for the serial interface to come up (user literally needs to open the serial monitor in the IDE).
     *
     * Will blink a signal (yellow 500ms) while waiting.
     */
    void wait()
    {
        auto& lights = dosa::Lights::getInstance();

        while (!Serial) {
            lights.set(true, true, true, true);
            delay(1500);
            lights.off();
            delay(1500);
        }
    }

   private:
    SerialComms()
    {
        Serial.begin(9600);
    }

    LogLevel loggingLevel = LogLevel::INFO;
};

}  // namespace dosa
