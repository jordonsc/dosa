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

// All times in milliseconds, see also dosa::settings.h for configurable values
#define MAX_DOOR_SEQ_TIME 20000  // Max time to open or close the door before declaring a system error
#define MOTOR_CPR_WARMUP 1000    // Grace we give the motor to report CPR pulses before declaring a stall
#define MOTOR_SLOW_SPEED 100     // Motor speed (1-255) to run when not at full speed (nearing apex)

// If defined, the door will continue to open until stopped (else it will open only to OPEN_HIGH_SPEED_TICKS)
// #define DOOR_FULL_OPEN

// If defined, we allow the door to be interrupted during the close sequence
// #define DOOR_CLOSE_ALLOW_INTERRUPT

namespace dosa::door {

namespace {

volatile unsigned long int_cpr_ticks;

/**
 * Called by hardware interrupt when CPR pulses.
 */
void intCprTick()
{
    ++int_cpr_ticks;
}

}  // end anonymous namespace

enum class DoorErrorCode : byte
{
    UNKNOWN = 0,
    OPEN_TIMEOUT = 1,
    CLOSE_TIMEOUT = 2,
    JAMMED = 3
};

typedef void (*winchErrorCallback)(DoorErrorCode, void*);
typedef bool (*doorInterruptCallback)(void*);

class DoorWinch : public Loggable
{
   protected:
   public:
    explicit DoorWinch(SerialComms* s, Settings& settings) : Loggable(s), kill_sw(PIN_REED_SW, true), settings(settings)
    {
        pinMode(PIN_MOTOR_A, OUTPUT);
        pinMode(PIN_MOTOR_B, OUTPUT);
        pinMode(PIN_MOTOR_PWM, OUTPUT);
        pinMode(PIN_MOTOR_CS, INPUT);
        pinMode(PIN_MOTOR_CPR, INPUT);

        attachInterrupt(digitalPinToInterrupt(PIN_MOTOR_CPR), intCprTick, RISING);
    }

    /**
     * Callback to be run when an error has occurred.
     */
    void setErrorCallback(winchErrorCallback cb, void* context = nullptr)
    {
        error_cb = cb;
        error_cb_ctx = context;
    }

    /**
     * If set, this callback will be continuously called during the open-wait and closing states of the door sequence.
     *
     * If this function returns true, the door will either reset its wait timer or halt closing and re-open, depending
     * on the state of the door.
     */
    void setInterruptCallback(doorInterruptCallback cb, void* context = nullptr)
    {
        interrupt_cb = cb;
        interrupt_cb_ctx = context;
    }

    /**
     * Trigger the door open/close sequence.
     */
    void trigger()
    {
        // Number of ticks the close sequence didn't complete before it was interrupted
        unsigned long deficit = 0;

        // Put the sequence in a loop as the door might re-open during the closing sequence
        while (true) {
            auto open_ticks = open(deficit);
            logln("Opened in " + String(open_ticks) + " ticks, open spread " + String(open_ticks + deficit));
            open_ticks += deficit;

            auto openWaitTimer = millis();
            while (millis() - openWaitTimer < settings.getDoorOpenWait()) {
                if (interrupt_cb != nullptr && interrupt_cb(interrupt_cb_ctx)) {
                    // Callback has asked us to reset open-wait timer (activity near door)
                    openWaitTimer = millis();
                }
                delay(10);
            }

            // Request the door close to the same degree as it was last opened + any deficit from previous iterations
            auto close_ticks = close(open_ticks);

            // If we fully closed, then return control to the main loop
            if (close_ticks >= open_ticks) {
                logln("Closed in " + String(close_ticks) + " ticks");
                break;
            } else {
                deficit = open_ticks - close_ticks;
                logln("Partial close for " + String(close_ticks) + " ticks, deficit " + String(deficit));
            }
        }

        uint32_t max_delay = settings.getDoorCoolDown() * 2;
        auto seq_complete_time = millis();
        while (millis() - seq_complete_time < max_delay) {
            if ((interrupt_cb == nullptr || !interrupt_cb(interrupt_cb_ctx)) &&
                (millis() - seq_complete_time > settings.getDoorCoolDown())) {
                // No movement detected, min delay exceeded - allow exit
                break;
            }
            delay(100);
        }
    }

    /**
     * Start door open sequence.
     *
     * Returns the number of CPR pulses throughout the sequence.
     */
    uint32_t open(unsigned long deficit = 0)
    {
        logln("Door: OPEN");
        seq_start_time = millis();
        resetCprTimer();
        kill_sw.process();

        uint32_t high_speed_ticks = settings.getDoorOpenTicks() - deficit;

        if (high_speed_ticks > 0) {
            logln("Running at full-speed for " + String(high_speed_ticks) + " ticks", dosa::LogLevel::DEBUG);

            // Full speed run at the start, this is when there is the most strain on the motor
            setMotor(true, 255);
            while (int_cpr_ticks < high_speed_ticks) {
                delay(10);
                if (checkForOpenKill()) {
                    stopMotor();
                    return int_cpr_ticks;
                }
            }
        }

#ifdef DOOR_FULL_OPEN
        // After we've opened part-way, slow down to avoid damage
        setMotor(true, MOTOR_SLOW_SPEED);
        while (!checkForOpenKill()) {
            delay(10);
        }
#endif

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

        setMotor(false, 255);

        while (!checkForCloseKill(ticks)) {
            delay(10);
#ifdef DOOR_CLOSE_ALLOW_INTERRUPT
            if (interrupt_cb != nullptr && interrupt_cb(interrupt_cb_ctx)) {
                break;
            }
#endif
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
    Switch kill_sw;
    Settings& settings;

    unsigned long seq_start_time = 0;  // Time that an open/close sequence started
    unsigned long cpr_last_time = 0;   // For calculating motor speed
    unsigned long cpr_last_ticks = 0;

    winchErrorCallback error_cb = nullptr;
    void* error_cb_ctx = nullptr;

    doorInterruptCallback interrupt_cb = nullptr;
    void* interrupt_cb_ctx = nullptr;

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
            stopMotor();
            if (error_cb != nullptr) {
                error_cb(DoorErrorCode::OPEN_TIMEOUT, error_cb_ctx);
            }
            return true;
        }

        // Check kill switch
        if (kill_sw.process() && kill_sw.getState()) {
            logln("Door hit kill switch");
            stopMotor();
            return true;
        }

        // Check for motor stall
        if (run_time > MOTOR_CPR_WARMUP && getTicksPerSecond() == 0) {
            logln("Door blocked while opening");
            stopMotor();
            return true;
        }

        return false;
    }

    bool checkForCloseKill(unsigned long ticks)
    {
        auto run_time = millis() - seq_start_time;

        // Correct way for close sequence to end: matched the same CPR pulses as open sequence
        if (int_cpr_ticks > ticks) {
            stopMotor();
            return true;
        }

        // Motor stall
        if (run_time > MOTOR_CPR_WARMUP && getTicksPerSecond() == 0) {
            logln("Winch jammed (close sequence!)", dosa::LogLevel::WARNING);
            stopMotor();
            if (error_cb != nullptr) {
                error_cb(DoorErrorCode::JAMMED, error_cb_ctx);
            }
            return true;
        }

        // Exceeded sequence max time
        if (run_time > MAX_DOOR_SEQ_TIME) {
            logln("Door close sequence max time exceeded", dosa::LogLevel::WARNING);
            stopMotor();
            if (error_cb != nullptr) {
                error_cb(DoorErrorCode::CLOSE_TIMEOUT, error_cb_ctx);
            }
            return true;
        }

        return false;
    }
};

}  // namespace dosa::door
