/**
 * Switch controller for local device.
 */

#pragma once

#include <ArduinoBLE.h>

#include "const.h"

#define SWITCH_DEFAULT_DEBOUNCE_THRESHOLD 100

namespace dosa {

typedef void (*switchCallback)(bool, void*);

/**
 * Any given open/close switch.
 *
 * Manages the state & debounce for an open/close switch and reports on that state. You may read the state in two ways:
 * 1. Call `process()` and if true (state has changed) then read state with `getState()`
 * 2. Set a callback and a call `process()` in a loop, which will call the callback when state has changed
 */
class Switch
{
   public:
    /**
     * @param p  Digital pin number
     * @param pu If the switch is configured for pull-up (else it must have a pull-down resistor)
     */
    Switch(uint8_t p, bool pu, unsigned long debounce = SWITCH_DEFAULT_DEBOUNCE_THRESHOLD)
        : pin(p),
          pull_up(pu),
          debounce_threshold(debounce)
    {
        if (pull_up) {
            pinMode(pin, INPUT_PULLUP);
        } else {
            pinMode(pin, INPUT);
        }
    }

    /**
     * Callback to be run when state has changed.
     */
    void setCallback(switchCallback cb, void* context = nullptr)
    {
        trigger_cb = cb;
        trigger_cb_ctx = context;
    }

    /**
     * Checks for state change.
     *
     * Returns true if the state has changed.
     */
    bool process()
    {
        if (millis() - last_poll < SWITCH_DEFAULT_DEBOUNCE_THRESHOLD) {
            return false;
        }

        last_poll = millis();
        bool s = digitalRead(pin) == 1;
        if (pull_up) {
            s = !s;
        }

        if (s != state) {
            state = s;
            if (trigger_cb != nullptr) {
                trigger_cb(state, trigger_cb_ctx);
            }
            return true;
        } else {
            return false;
        }
    }

    /**
     * Get the last known state of the switch.
     *
     * Returns true if the switch is closed. To update the state, first call `process()`.
     */
    [[nodiscard]] bool getState() const
    {
        return state;
    }

   protected:
    uint8_t pin;
    bool pull_up;
    bool state = false;
    unsigned long last_poll = 0;
    unsigned long debounce_threshold;

    switchCallback trigger_cb = nullptr;
    void* trigger_cb_ctx = nullptr;
};

}  // namespace dosa
