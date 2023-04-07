#pragma once

#include <Arduino.h>

#include "const.h"

namespace dosa {

class PwmApp final
{
   public:
    void init()
    {
        Serial.begin(9600);
        Serial.flush();
    }

    void loop()
    {
        if (Serial.available() > 0) {
            auto i = Serial.read();
            if (i < 0 || i > 100) {
                Serial.println("ERR: out of range");
            } else {
                // Set PWM
                Serial.println("OK: " + String(i));
            }
        } else {
            delay(10);
        }
    }
};

}  // namespace dosa
