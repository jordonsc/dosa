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
#define PIN_REED_SW 7

namespace dosa::door {

class DoorWinch : public Loggable
{
   public:
    explicit DoorWinch(SerialComms* s) : Loggable(s), kill_sw(PIN_REED_SW, true)
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
        delay(5000);
        close();
    }

    /**
     * Start door open sequence
     */
    void open()
    {
        logln("Door: OPEN");

        kill_sw.process();

        // Start sequence: slowly increase speed of winch
        for (unsigned short pwr = 10; pwr < 250; pwr += 10) {
            setMotor(true, pwr);
            delay(100);
            if (checkForKill()) {
                return;
            }
        }

        setMotor(true, 255);
        logln("Full drive, current: " + String(getMotorCurrent()), dosa::LogLevel::DEBUG);

        // Full-drive sequence: continue at full speed
        for (short i = 0; i < 200; ++i) {
            delay(10);
            if (checkForKill()) {
                return;
            }
        }

        // Approaching sequence: slow down as the door approaches apex
        for (unsigned short pwr = 250; pwr > 50; pwr -= 10) {
            setMotor(true, pwr);
            delay(100);
            if (checkForKill()) {
                return;
            }
        }

        // Final sequence: continue on slowly, monitoring kill switch and current
        for (short i = 0; i < 1000; ++i) {
            delay(10);
            if (checkForKill()) {
                return;
            }
        }

        stopMotor();
    }

    /**
     * Release the door, allowing it to close fully.
     */
    void close()
    {
        logln("Door: CLOSE");

        // Start sequence: ramp up to full speed
        for (unsigned short pwr = 10; pwr < 250; pwr += 10) {
            setMotor(false, pwr);
            delay(100);
        }

        // Final sequence: release belt at full speed for calculated time
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
    unsigned long motor_panic_time = 0;
    dosa::Switch kill_sw;

    /**
     * Check if the door winch should be halted.
     *
     * This will occur if either the kill switch is closed, or if the motor current exceeds a threshold.
     */
    bool checkForKill()
    {
        // Check kill switch
        if (kill_sw.process() && kill_sw.getState()) {
            logln("Door hit kill switch");
            stopMotor();
            return true;
        }

        return false;
    }
};

}  // namespace dosa::door
