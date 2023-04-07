#pragma once

#include <Arduino.h>

#include "const.h"
#include "megaAVR_PWM.h"

namespace dosa {

class PwmApp final
{
   public:
    void init()
    {
        Serial.begin(9600);
        Serial.flush();

        pwm = new megaAVR_PWM(DOSA_PWM_PIN, DOSA_PWM_FREQ, 10);
        pwm->setPWM();
    }

    void loop()
    {
        if (Serial.available() > 0) {
            auto i = Serial.read();
            if (i < 0 || i > 100) {
                Serial.println("ERR: out of range");
            } else {
                // Set PWM
                pwm->setPWM(DOSA_PWM_PIN, DOSA_PWM_FREQ, float(i));
                Serial.println("OK: " + String(i));
            }
        } else {
            delay(10);
        }
    }

   private:
    megaAVR_PWM* pwm;
};

}  // namespace dosa
