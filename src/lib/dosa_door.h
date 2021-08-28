/**
 * Client class for door motor controls on the local device. This should be used exclusively with the door master
 * device.
 */

#pragma once

#include <ArduinoBLE.h>

#include "dosa_const.h"
#include "dosa_serial.h"

#define PIN_MOTOR_A 2
#define PIN_MOTOR_B 3
#define PIN_MOTOR_PWM 6
#define PIN_MOTOR_CS 21

namespace dosa {

class Door
{
   public:
    Door() : serial(dosa::SerialComms::getInstance())
    {
        pinMode(PIN_MOTOR_A, OUTPUT);
        pinMode(PIN_MOTOR_B, OUTPUT);
        pinMode(PIN_MOTOR_PWM, OUTPUT);
        pinMode(PIN_MOTOR_CS, INPUT);
    }

    /**
     * Trigger the door open/close sequence.
     *
     * Do not manually call lower-level functions during this sequence.
     */
    void trigger() {}

    /**
     * Start door open sequence
     */
    void open() {}

    /**
     * Release the door, allowing it to close fully.
     */
    void close() {}

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

    void motorTest()
    {
        serial.writeln("\nBEGIN MOTOR TEST");

        serial.writeln("Forward 10");
        setMotor(true, 10);
        delay(1000);
        serial.writeln("Current: " + String(getMotorCurrent()));
        delay(1000);

        serial.writeln("Forward 100");
        setMotor(true, 100);
        delay(1000);
        serial.writeln("Current: " + String(getMotorCurrent()));
        delay(1000);

        serial.writeln("Forward 255");
        setMotor(true, 255);
        delay(1000);
        serial.writeln("Current: " + String(getMotorCurrent()));
        delay(1000);

        serial.writeln("Stop");
        stopMotor();
        delay(1000);
        serial.writeln("Current: " + String(getMotorCurrent()));
        delay(1000);

        serial.writeln("Reverse 255");
        setMotor(false, 255);
        delay(1000);
        serial.writeln("Current: " + String(getMotorCurrent()));
        delay(1000);

        stopMotor();

        serial.writeln("MOTOR TEST COMPLETE\n");
    }

   protected:
    unsigned short state = 0;
    dosa::SerialComms& serial;
};

}  // namespace dosa
