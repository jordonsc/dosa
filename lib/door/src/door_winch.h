/**
 * Client class for door motor controls on the local device. This should be used exclusively with the door master
 * device.
 */

#pragma once

#include <ArduinoBLE.h>
#include <dosa.h>

#define PIN_MOTOR_A 2
#define PIN_MOTOR_B 3
#define PIN_MOTOR_PWM 6
#define PIN_MOTOR_CS 21

namespace dosa::door {

class DoorWinch : public Loggable
{
   public:
    explicit DoorWinch(SerialComms* s) : Loggable(s)
    {
        pinMode(PIN_MOTOR_A, OUTPUT);
        pinMode(PIN_MOTOR_B, OUTPUT);
        pinMode(PIN_MOTOR_PWM, OUTPUT);
        pinMode(PIN_MOTOR_CS, INPUT);
    }

    /**
     * Trigger the door open/close sequence.
     */
    void trigger()
    {
        open();
        delay(2000);
        close();
    }

    /**
     * Start door open sequence
     */
    void open()
    {
        logln("Door: OPEN");

        for (unsigned short pwr = 10; pwr < 250; pwr += 10) {
            setMotor(true, pwr);
            delay(100);
        }
        setMotor(true, 255);
        logln("Full drive, current: " + String(getMotorCurrent()), dosa::LogLevel::DEBUG);

        delay(2000);

        for (unsigned short pwr = 250; pwr > 50; pwr -= 10) {
            setMotor(true, pwr);
            delay(100);
        }

        delay(2000);
        stopMotor();
    }

    /**
     * Release the door, allowing it to close fully.
     */
    void close()
    {
        logln("Door: CLOSE");

        for (unsigned short pwr = 10; pwr < 250; pwr += 10) {
            setMotor(false, pwr);
            delay(100);
        }

        setMotor(false, 255);
        logln("Full drive, current: " + String(getMotorCurrent()), dosa::LogLevel::DEBUG);

        delay(3000);

        stopMotor();
    }

    void stopMotor()
    {
        digitalWrite(PIN_MOTOR_A, LOW);
        digitalWrite(PIN_MOTOR_B, LOW);
        digitalWrite(PIN_MOTOR_PWM, 0);
    }

    void setMotor(bool forward, int power)
    {
        if (forward) {
            digitalWrite(PIN_MOTOR_A, HIGH);
            digitalWrite(PIN_MOTOR_B, LOW);
        } else {
            digitalWrite(PIN_MOTOR_A, LOW);
            digitalWrite(PIN_MOTOR_B, HIGH);
        }

        analogWrite(PIN_MOTOR_PWM, power);
    }

    int getMotorCurrent()
    {
        return analogRead(PIN_MOTOR_CS);
    }

   protected:
    unsigned short state = 0;
};

}  // namespace dosa::door
