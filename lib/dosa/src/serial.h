/**
 * Serial communications (USB).
 */

#pragma once

#include <Arduino.h>

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
    SerialComms()
    {
        Serial.begin(9600);
    }

    void setLogLevel(LogLevel level)
    {
        loggingLevel = level;
    }

    /**
     * Write to serial interface.
     */
    void write(String const& msg, LogLevel level = LogLevel::INFO) const
    {
        if (level >= loggingLevel) {
            Serial.print(msg);
        }
    }

    /**
     * Write to serial interface, include a trailing new-line.
     */
    void writeln(String const& msg, LogLevel level = LogLevel::INFO) const
    {
        if (level >= loggingLevel) {
            Serial.println(msg);
        }
    }

    /**
     * Waits for the read activity on the serial interface.
     *
     * This is normally achieved using the `./dosa monitor` command or the 'serial monitor' in the IDE.
     */
    void wait()
    {
        while (!Serial) {
            delay(100);
        }
    }

   private:
    LogLevel loggingLevel = LogLevel::INFO;
};

}  // namespace dosa
