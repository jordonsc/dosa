/**
 * Client class for door motor controls on the local device. This should be used exclusively with the door master
 * device.
 */

#pragma once

#include <ArduinoBLE.h>
#include <dosa.h>

#define PIN_MOTOR_A 4    // Motor output fwd
#define PIN_MOTOR_B 5    // Motor output reverse
#define PIN_MOTOR_PWM 6  // Motor speed
#define PIN_REED_SW 7    // Kill switch
#define PIN_MOTOR_CPR 2  // Motor CPR encoder pulse
#define PIN_MOTOR_CS 21  // Motor analogue current input

#define MAX_DOOR_SEQ_TIME 15000  // Max time in milliseconds to open or close the door
#define MOTOR_CPR_WARMUP 1000    // Number of milliseconds grace we give the motor to report CPR pulses

namespace dosa::door {

namespace {

volatile unsigned long int_cpr_ticks;

/**
 * Called by interrupt when CPR pulses.
 */
void intCprTick()
{
    ++int_cpr_ticks;
}

}  // end anonymous namespace

class DoorWinch : public Loggable
{
   protected:
   public:
    explicit DoorWinch(SerialComms* s) : Loggable(s), kill_sw(PIN_REED_SW, true)
    {
        pinMode(PIN_MOTOR_A, OUTPUT);
        pinMode(PIN_MOTOR_B, OUTPUT);
        pinMode(PIN_MOTOR_PWM, OUTPUT);
        pinMode(PIN_MOTOR_CS, INPUT);
        pinMode(PIN_MOTOR_CPR, INPUT);

        attachInterrupt(digitalPinToInterrupt(PIN_MOTOR_CPR), intCprTick, RISING);
    }

    /**
     * Trigger the door open/close sequence.
     */
    void trigger()
    {
        auto open_ticks = open();
        logln("Opened in " + String(open_ticks) + " ticks");
        delay(5000);
        auto close_ticks = close(open_ticks);
        logln("Closed in " + String(close_ticks) + " ticks");
    }

    /**
     * Start door open sequence.
     *
     * Returns the number of CPR pulses throughout the sequence.
     */
    unsigned long open()
    {
        logln("Door: OPEN");
        seq_start_time = millis();
        resetCprTimer();
        kill_sw.process();

        // Start sequence: slowly increase speed of winch
        for (unsigned short pwr = 10; pwr < 250; pwr += 10) {
            setMotor(true, pwr);
            delay(50);
            if (checkForOpenKill()) {
                stopMotor();
                return int_cpr_ticks;
            }
        }

        setMotor(true, 255);

        // Full-drive sequence: continue at full speed
        for (short i = 0; i < 100; ++i) {
            delay(10);
            if (checkForOpenKill()) {
                stopMotor();
                return int_cpr_ticks;
            }
        }

        // Approaching sequence: slow down as the door approaches apex
        for (unsigned short pwr = 250; pwr > 50; pwr -= 10) {
            setMotor(true, pwr);
            delay(50);
            if (checkForOpenKill()) {
                stopMotor();
                return int_cpr_ticks;
            }
        }

        // Final sequence: continue on slowly, monitoring kill conditions
        while (!checkForOpenKill()) {
            delay(10);
        }

        stopMotor();
        return int_cpr_ticks;
    }

    /**
     * Release the door for `ticks` number of CPR pulses. Should match the open sequence.
     *
     * Returns number of CPR pulses during the process.
     */
    unsigned long close(unsigned long ticks)
    {
        logln("Door: CLOSE");
        seq_start_time = millis();
        resetCprTimer();

        // Start sequence: ramp up to full speed
        for (unsigned short pwr = 10; pwr < 250; pwr += 10) {
            setMotor(false, pwr);
            delay(100);

            if (checkForCloseKill(ticks)) {
                stopMotor();
                return int_cpr_ticks;
            }
        }

        // Final sequence: release belt at full speed until ticks are consumed.
        setMotor(false, 255);
        delay(10);

        while (!checkForCloseKill(ticks)) {
            delay(10);
        }

        stopMotor();
        return int_cpr_ticks;
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

    /**
     * Check the motor current from analogue pin.
     */
    [[maybe_unused]] int getMotorCurrent()
    {
        return analogRead(PIN_MOTOR_CS);
    }

   protected:
    unsigned short state;  // Motor state
    dosa::Switch kill_sw;

    unsigned long seq_start_time = 0;  // Time that an open/close sequence started
    unsigned long cpr_last_time = 0;   // For calculating motor speed
    unsigned long cpr_last_ticks = 0;

    /**
     * Resets and initialises tick-data so that ticksPerSecond may be accurately called.
     */
    void resetCprTimer()
    {
        int_cpr_ticks = 0;
        cpr_last_ticks = 0;
        cpr_last_time = millis();
    }

    /**
     * Returns the number of CPR ticks per second.
     *
     * Calculated by determining the number of ticks since this function was last called. Call resetCprTimer() before
     * recording tick-rate.
     */
    double getTicksPerSecond()
    {
        auto now = millis();
        if (now == cpr_last_time) {
            // div by zero safety
            return 1;
        }

        double coefficient = (double)1000 / (double)(now - cpr_last_time);
        double tps = (double)(int_cpr_ticks - cpr_last_ticks) * coefficient;

        cpr_last_time = now;
        cpr_last_ticks = int_cpr_ticks;

        return tps;
    }

    /**
     * Check if the door winch should be halted.
     *
     * This will occur if either the kill switch is closed, or if the motor speed is reduced to zero (ie jammed). This
     * function should be called after at least some time since resetCprTimer() to prevent is thinking the door has
     * been immediately jammed.
     */
    bool checkForOpenKill()
    {
        auto run_time = millis() - seq_start_time;

        // Exceeded sequence max time
        if (run_time > MAX_DOOR_SEQ_TIME) {
            logln("Door open sequence max time exceeded");
            return true;
        }

        // Check kill switch
        if (kill_sw.process() && kill_sw.getState()) {
            logln("Door hit kill switch");
            return true;
        }

        // Check for motor stall
        if (run_time > MOTOR_CPR_WARMUP && getTicksPerSecond() == 0) {
            logln("Door blocked while opening");
            return true;
        }

        return false;
    }

    bool checkForCloseKill(unsigned long ticks)
    {
        auto run_time = millis() - seq_start_time;

        // Correct way for close sequence to end: matched the same CPR pulses as open sequence
        if (int_cpr_ticks > ticks) {
            return true;
        }

        // Motor stall
        if (run_time > MOTOR_CPR_WARMUP && getTicksPerSecond() == 0) {
            logln("Winch jammed (close sequence!)", dosa::LogLevel::WARNING);
            return true;
        }

        // Exceeded sequence max time
        if (run_time > MAX_DOOR_SEQ_TIME) {
            logln("Door close sequence max time exceeded", dosa::LogLevel::WARNING);
            return true;
        }

        return false;
    }
};

}  // namespace dosa::door
