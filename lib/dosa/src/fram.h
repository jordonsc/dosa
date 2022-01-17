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
    explicit Fram(SerialComms* s = nullptr) : Loggable(s) {
        // Why a pointer? for some reason IDEs aren't picking up the first constructor for this class which is cascading
        // issues downstream. This averts this issue and allows other classes to register correctly.
        ram = new Adafruit_FRAM_SPI(FRAM_CS_PIN);

        // Don't do fram.begin() here as serial won't be ready and we'll miss errors
    }

    virtual ~Fram() {
        delete ram;
    }

    void init()
    {
        inited = true;

        logln("Begin FRAM init..");
        if (!ram->begin()) {
            logln("ERROR: FRAM failed to initialise!", dosa::LogLevel::ERROR);
        }
    }

    void write(String const& v)
    {
        if (!inited) {
            init();
        }

        ram->writeEnable(true);
        ram->write(0, (uint8_t*)v.c_str(), v.length() + 1);
        ram->writeEnable(false);
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
            c = ram->read8(addr);
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
        return *ram;
    }

   protected:
    bool inited = false;
    Adafruit_FRAM_SPI* ram = nullptr;
};

}  // namespace dosa
