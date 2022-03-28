#pragma once

#include <Arduino.h>

#include "serial.h"

namespace dosa {

class Loggable
{
   public:
    explicit Loggable(SerialComms* s = nullptr) : serial(s) {}

   protected:
    void log(String const& s, dosa::LogLevel lvl = dosa::LogLevel::INFO) const
    {
        if (serial != nullptr) {
            serial->write(s, lvl);
        }
    }

    void logln(String const& s, dosa::LogLevel lvl = dosa::LogLevel::INFO) const
    {
        if (serial != nullptr) {
            serial->writeln(s, lvl);
        }
    }

    void setSerial(SerialComms* s)
    {
        serial = s;
    }

    SerialComms* serial;
};

}  // namespace dosa
