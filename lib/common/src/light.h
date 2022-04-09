#pragma once

#include <Arduino.h>

namespace dosa {

class Light
{
   public:
    Light(uint16_t pin, bool active = false) : pin(pin)
    {
        pinMode(pin, OUTPUT);
        setState(active);
    }

    Light(uint16_t pin, uint32_t on, uint32_t off, bool active = false) : pin(pin)
    {
        pinMode(pin, OUTPUT);
        setSequence(on, off);
        setState(active);
    }

    void setSequence(uint32_t on, uint32_t off)
    {
        seq_on_time = on;
        seq_off_time = off;
    }

    void begin(uint32_t seq_time)
    {
        sequence_end = millis() + seq_time;
        setState(true);
    }

    void end()
    {
        sequence_end = 0;
        setState(false);
    }

    void process()
    {
        if (sequence_end == 0) {
            return;
        }

        // Sequence time expired, turn off and disable sequence
        if (sequence_end < millis()) {
            setState(false);
            sequence_end = 0;
            return;
        }

        if (state) {
            // Sequence on
            if (seq_off_time > 0 && (millis() - state_changed > seq_on_time)) {
                setState(false);
            }
        } else {
            // Sequence off
            if (millis() - state_changed > seq_off_time) {
                setState(true);
            }
        }
    }

    void on()
    {
        sequence_end = 0;
        setState(true);
    }

    void off()
    {
        sequence_end = 0;
        setState(false);
    }

    [[nodiscard]] bool getState() const
    {
        return state;
    }

   protected:
    void setState(bool active)
    {
        digitalWrite(pin, active ? HIGH : LOW);
        state = active;
        state_changed = millis();
    }

    uint16_t pin;
    bool state;
    uint32_t state_changed;
    uint32_t sequence_end = 0;
    uint32_t seq_on_time = 1000;
    uint32_t seq_off_time = 500;
};

}  // namespace dosa