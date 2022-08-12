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
#define PIN_MOTOR_CPR 2  // Motor CPR encoder pulse
#define PIN_MOTOR_CS 21  // Motor analogue current input

// All times in milliseconds, see also dosa::settings.h for configurable values
#define MAX_DOOR_SEQ_TIME 20000  // Max time to open or close the door before declaring a system error
#define MOTOR_CPR_WARMUP 1000    // Grace we give the motor to report CPR pulses before declaring a stall
#define STALL_PERIOD 500         // Time a motor cannot move before considering stalled
#define SONAR_MAX_WAIT 1000      // Max time we wait for the sonar to report before declaring an error

// If defined, we allow the door to be interrupted during the close sequence
// #define DOOR_CLOSE_ALLOW_INTERRUPT

// Calibration thresholds
#define WINCH_FALLBACK_OPEN_COEFFICIENT 0.7
#define WINCH_FALLBACK_CLOSE_COEFFICIENT 1.0
#define WINCH_CALIBRATE_TIMEOUT 3500
#define WINCH_CALIBRATE_TENSION_COEFFICIENT 0.7
#define WINCH_CALIBRATE_ROLLBACK_TICKS 300

// Forced fallback mode (use if sonar absent)
#define DOOR_SONAR_FALLBACK 1

namespace dosa {

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
    JAMMED = 3,
    SONAR_ERROR = 4,
    CALIBRATE_TIMEOUT = 5
};

typedef void (*tickCallback)(void*);
typedef void (*winchErrorCallback)(DoorErrorCode, void*);
typedef bool (*doorInterruptCallback)(void*);

class DoorWinch : public Loggable
{
   protected:
   public:
    explicit DoorWinch(SerialComms* s, Settings& settings, Sonar& sonar) : Loggable(s), settings(settings), sonar(sonar)
    {
        pinMode(PIN_MOTOR_A, OUTPUT);
        pinMode(PIN_MOTOR_B, OUTPUT);
        pinMode(PIN_MOTOR_PWM, OUTPUT);
        pinMode(PIN_MOTOR_CS, INPUT);
        pinMode(PIN_MOTOR_CPR, INPUT);

        attachInterrupt(digitalPinToInterrupt(PIN_MOTOR_CPR), intCprTick, RISING);
    }

    /**
     * Called every cycle to allow the main application to do some processing.
     *
     * SHOULD NOT BLOCK. QUICK OPERATIONS ONLY.
     */
    void setTickCallback(tickCallback cb, void* context = nullptr)
    {
        tick_cb = cb;
        tick_cb_ctx = context;
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
        auto open_ticks = open();
        // If the sensor believes the door is already fully open, it will return 0 ticks (and have done nothing).
        if (open_ticks > 0) {
            logln("Opened in " + String(open_ticks) + " ticks");
        }

        // Open-wait
        auto openWaitTimer = millis();
        while (millis() - openWaitTimer < settings.getDoorOpenWait()) {
            runTickCallback();
            if (interrupt_cb != nullptr && interrupt_cb(interrupt_cb_ctx)) {
                // Callback has asked us to reset open-wait timer (activity near door)
                openWaitTimer = millis();
            }
            delay(10);
        }

        // Request the door close to the same degree as it was last opened + a coefficient to ensure the pulley is slack
        auto close_ticks = close(
            sonar_fault ? settings.getDoorCloseTicks() * WINCH_FALLBACK_CLOSE_COEFFICIENT
                        : settings.getDoorCloseTicks());
        logln("Closed in " + String(close_ticks) + " ticks");

        // Remove slack on the line
        delay(1000);
        calibrate();

        // Close-wait (cool-down)
        uint32_t max_delay = settings.getDoorCoolDown() * 2;
        auto seq_complete_time = millis();
        while (millis() - seq_complete_time < max_delay) {
            runTickCallback();
            if ((interrupt_cb == nullptr || !interrupt_cb(interrupt_cb_ctx)) &&
                (millis() - seq_complete_time > settings.getDoorCoolDown())) {
                // Sensors clear & min delay exceeded - allow exit
                break;
            }
            delay(10);
        }
    }

    /**
     * Start door open sequence.
     *
     * Returns the number of CPR pulses throughout the sequence.
     */
    uint32_t open()
    {
        logln("Door: OPEN");
        seq_start_time = millis();
        resetCprTimer();
        uint32_t open_distance = settings.getDoorOpenDistance();

        // if the sonar fails, we'll use the door close ticks as a time-based open sequence
#ifndef DOOR_SONAR_FALLBACK
        sonar_fault = !waitForSonarReady();
#else
        sonar_fault = true;
#endif
        auto open_ticks = settings.getDoorCloseTicks() * WINCH_FALLBACK_OPEN_COEFFICIENT;

        if (!sonar_fault && (sonar.getDistance() > 0 && sonar.getDistance() < open_distance)) {
            logln("Door open-jam detected, skipping open sequence", LogLevel::WARNING);
            return 0;
        }

        setMotor(true, 255);
        while (true) {
            runTickCallback();
            if (sonar_fault) {
                // Legacy/fallback mode
                if (checkForOpenKill() || (int_cpr_ticks > open_ticks)) {
                    logln("Open halted at " + String(int_cpr_ticks) + " ticks", LogLevel::DEBUG);
                    break;
                }
            } else {
                // Sonar apex detection
                sonar.process();
                if (checkForOpenKill() || (sonar.getDistance() > 0 && sonar.getDistance() < open_distance)) {
                    logln("Open halted at " + String(sonar.getDistance()) + "mm", LogLevel::DEBUG);
                    break;
                }
            }
        }

        stopMotor();
        return int_cpr_ticks;
    }

    /**
     * Release the door for `ticks` number of CPR pulses. Should be a little above what is required to fully close
     * the door from it's maximum draw angle.
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
            runTickCallback();
        }

        stopMotor();
        return int_cpr_ticks;
    }

    /**
     * Opens the door until tension is detected, then rollback slightly.
     *
     * The purpose of this is to remove slack on the line, which allows the next open sequence to be consistent.
     */
    void calibrate()
    {
        logln("Door: CALIBRATE");
        auto start_time = millis();
        resetCprTimer();

        setMotor(true, 255);

        double max_tps = 0;
        while (true) {
            delay(10);
            auto tps = getTicksPerSecond();
            if (tps > max_tps) {
                // Speed is increasing
                max_tps = tps;
            } else if (max_tps > 0 && (tps < (max_tps * WINCH_CALIBRATE_TENSION_COEFFICIENT))) {
                // Tension detected, drop out
                break;
            } else if (millis() - start_time > WINCH_CALIBRATE_TIMEOUT) {
                // Tension never detected, alert and drop out
                logln("Calibrate timeout");
                if (error_cb != nullptr) {
                    error_cb(DoorErrorCode::CALIBRATE_TIMEOUT, error_cb_ctx);
                }
                break;
            }
        }

        stopMotor();
        delay(100);
        resetCprTimer();
        setMotor(false, 255);
        start_time = millis();

        // Rollback a little to undo the door breach during tension testing
        while (int_cpr_ticks < WINCH_CALIBRATE_ROLLBACK_TICKS) {
            delay(10);

            // This is unlikely, but always have a timeout just in case
            if (millis() - start_time > WINCH_CALIBRATE_TIMEOUT) {
                logln("Calibrate timeout");
                if (error_cb != nullptr) {
                    error_cb(DoorErrorCode::CALIBRATE_TIMEOUT, error_cb_ctx);
                }
                break;
            }
        }

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

    /**
     * Check the motor current from analogue pin.
     */
    [[maybe_unused]] int getMotorCurrent()
    {
        return analogRead(PIN_MOTOR_CS);
    }

   protected:
    Settings& settings;
    Sonar& sonar;

    unsigned long seq_start_time = 0;  // Time that an open/close sequence started
    unsigned long cpr_last_time = 0;   // For calculating motor speed
    unsigned long cpr_last_ticks = 0;

    winchErrorCallback error_cb = nullptr;
    void* error_cb_ctx = nullptr;

    doorInterruptCallback interrupt_cb = nullptr;
    void* interrupt_cb_ctx = nullptr;

    tickCallback tick_cb = nullptr;
    void* tick_cb_ctx = nullptr;

    bool sonar_fault = false;

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
        static uint32_t stall_time = 0;

        // Exceeded sequence max time
        if (run_time > MAX_DOOR_SEQ_TIME) {
            logln("Door open sequence max time exceeded");
            stopMotor();
            if (error_cb != nullptr) {
                error_cb(DoorErrorCode::OPEN_TIMEOUT, error_cb_ctx);
            }
            return true;
        }

        // Check for motor stall
        if (run_time > MOTOR_CPR_WARMUP && getTicksPerSecond() == 0) {
            if (stall_time > 0) {
                if (millis() - stall_time > STALL_PERIOD) {
                    logln("Door blocked while opening");
                    stopMotor();
                    return true;
                }
            } else {
                stall_time = millis();
            }
        } else {
            stall_time = 0;
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

    /**
     * Wait until we've had a distance report from the sonar.
     *
     * This should be called before trying to read the sonar distance for the first time to ensure the value is valid.
     */
    bool waitForSonarReady()
    {
        auto start_time = millis();
        while (millis() - start_time < SONAR_MAX_WAIT) {
            if (sonar.process()) {
                return true;
            }
        }

        logln("Sonar not reporting data!", LogLevel::ERROR);

        if (error_cb != nullptr) {
            error_cb(DoorErrorCode::SONAR_ERROR, error_cb_ctx);
        }

        return false;
    }

    void runTickCallback()
    {
        if (tick_cb != nullptr) {
            tick_cb(tick_cb_ctx);
        }
    }
};

}  // namespace dosa
