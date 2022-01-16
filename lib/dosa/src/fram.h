/**
 * Adafruit FRAM via SPI.
 */

#pragma once

#include <Adafruit_FRAM_SPI.h>

#include "loggable.h"

#ifndef FRAM_CS_PIN
#define FRAM_CS_PIN 10
#endif

namespace dosa {

class Fram : public Loggable
{
   public:
    explicit Fram(SerialComms* s = nullptr) : Loggable(s), fram(FRAM_CS_PIN) {
        // Don't do init here as serial won't be ready and we'll miss errors
    }

    void init()
    {
        inited = true;

        logln("Begin FRAM init..");
        if (!fram.begin()) {
            logln("ERROR: FRAM failed to initialise!", dosa::LogLevel::ERROR);
        }
    }

    void write(String const& v)
    {
        if (!inited) {
            init();
        }

        fram.writeEnable(true);
        fram.write(0, (uint8_t*)v.c_str(), v.length() + 1);
        fram.writeEnable(false);
    }

    String read()
    {
        if (!inited) {
            init();
        }
        
        String v;
        uint32_t addr = 0;
        uint8_t c;

        while (true) {
            c = fram.read8(addr);
            if (c == 0) {
                break;
            } else {
                v += (char)c;
            }

            if (++addr >= 8192) {
                break;
            }
        }

        return v;
    }

    Adafruit_FRAM_SPI& getFram()
    {
        return fram;
    }

   protected:
    bool inited = false;
    Adafruit_FRAM_SPI fram;
};

}  // namespace dosa
