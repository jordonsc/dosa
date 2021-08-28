/**
 * Switch controller for local device.
 *
 * TODO: state change, debounce delays.
 */

#pragma once

#include <ArduinoBLE.h>

#include "dosa_const.h"
#include "dosa_serial.h"

#define SWITCH_DEBOUNCE_THRESHOLD 200

namespace dosa {

class Switch
{
   public:
    explicit Switch(uint8_t p) : pin(p), serial(dosa::SerialComms::getInstance())
    {
        pinMode(pin, INPUT);
    }

    /**
     * Checks for state change, triggers callback on change.
     */
    void process() {}

    /**
     * Get the state of the switch.
     *
     * @return
     */
    bool getState()
    {
        return digitalRead(pin) == 1;
    }

   protected:
    uint8_t pin;
    uint8_t state = 0;
    dosa::SerialComms& serial;
};

}  // namespace dosa
