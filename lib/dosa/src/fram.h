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
    explicit Fram(SerialComms* s = nullptr) : Loggable(s)
    {
        // Why a pointer? for some reason IDEs aren't picking up the first constructor for this class which is cascading
        // issues downstream. This averts this issue and allows other classes to register correctly.
        ram = new Adafruit_FRAM_SPI(FRAM_CS_PIN);

        // Don't do fram.begin() here as serial won't be ready and we'll miss errors
    }

    virtual ~Fram()
    {
        delete ram;
    }

    void init()
    {
        inited = true;

        logln("Bringing FRAM chip online..");
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

    /**
     * Read the 4-byte header at position 0.
     */
    String readHeader()
    {
        if (!inited) {
            init();
        }

        uint8_t header[5] = {0};
        ram->read(0, header, 4);
        return String((char*)header);
    }

    void read(uint32_t addr, uint8_t* dest, size_t size)
    {
        if (!inited) {
            init();
        }

        ram->read(addr, dest, size);
    }

    String readUntilNull(uint32_t addr)
    {
        if (!inited) {
            init();
        }

        String v;
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

    void write(uint32_t addr, uint8_t const* payload, size_t size)
    {
        if (!inited) {
            init();
        }

        ram->writeEnable(true);
        ram->write(addr, (uint8_t*)payload, size);
        ram->writeEnable(false);
    }

    void write(uint32_t addr, String const& data, bool incl_null = false)
    {
        if (!inited) {
            init();
        }

        ram->writeEnable(true);
        ram->write(addr, (uint8_t*)data.c_str(), incl_null ? data.length() : data.length() + 1);
        ram->writeEnable(false);
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
