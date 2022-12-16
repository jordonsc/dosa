/**
 * Client class for door motor controls on the local device. This should be used exclusively with the door master
 * device.
 */

#pragma once

#include <ArduinoBLE.h>
#include <dosa.h>

#include <utility>

#define PIN_MOTOR_A 4    // Motor output fwd
#define PIN_MOTOR_B 5    // Motor output reverse
#define PIN_MOTOR_PWM 6  // Motor speed
#define PIN_MOTOR_CPR 2  // Motor CPR encoder pulse
#define PIN_MOTOR_CS 21  // Motor analogue current input

// All times in milliseconds, see also dosa::settings.h for configurable values
#define MAX_DOOR_SEQ_TIME 15000  // Max time to open or close the door before declaring a system error
#define MOTOR_CPR_WARMUP 100     // Grace we give the motor to report CPR pulses before declaring a stall
#define SONAR_MAX_WAIT 100       // Max time we wait for the sonar to report before declaring an error

// If defined, we allow the door to be interrupted during the close sequence
// #define DOOR_CLOSE_ALLOW_INTERRUPT

// Calibration thresholds
#define WINCH_FALLBACK_OPEN_COEFFICIENT 0.9
#define WINCH_FALLBACK_CLOSE_COEFFICIENT 1.1
#define WINCH_STALL_COEFFICIENT 0.2
#define WINCH_CALIBRATE_TIMEOUT 3000
#define WINCH_CALIBRATE_TENSION_COEFFICIENT 0.75
#define WINCH_CALIBRATE_ROLLBACK_TICKS 800
#define WINCH_CALIBRATE_PRE_DELAY 500

// Door power (0-255)
#define WINCH_MAX_POWER 200

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
typedef void (*netLogCallback)(String const&, NetLogLevel, void*);

class DoorWinch : public Loggable
{
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
     * Allow the winch to send to the NetLog.
     */
    void setNetLogCallback(netLogCallback cb, void* context = nullptr)
    {
        net_log_cb = cb;
        net_log_cb_ctx = context;
    }

    void netLog(String const& msg, NetLogLevel log_level = NetLogLevel::INFO)
    {
        if (net_log_cb != nullptr) {
            net_log_cb(msg, log_level, net_log_cb_ctx);
        }
    }

    /**
     * Trigger the door open/close sequence.
     */
    void trigger()
    {
        auto open_ticks = open();
        // If the sensor believes the door is already fully open, it will return 0 ticks (and have done nothing).
        if (open_ticks > 0) {
            netLog("Opened in " + String(open_ticks) + " ticks", NetLogLevel::DEBUG);
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
        auto close_spread = static_cast<unsigned long>(
            fallback_mode ? settings.getDoorCloseTicks() * WINCH_FALLBACK_CLOSE_COEFFICIENT
                          : settings.getDoorCloseTicks());

        if (require_extended_close) {
            // Normally because of a door jam, we need to undo the damage from opening too far
            close_spread *= static_cast<unsigned long>(1.5);
        }

        auto close_ticks = close(close_spread);
        netLog("Closed in " + String(close_ticks) + " ticks", NetLogLevel::DEBUG);

        // Remove slack on the line, cooldown
        delay(WINCH_CALIBRATE_PRE_DELAY);
        calibrate();
        cooldown();
    }

    /**
     * Manual request to close the door by a fractional amount.
     */
    void rewind(unsigned long rewind_ticks)
    {
        // Request the door close to specified ticks
        auto close_ticks = close(rewind_ticks);
        netLog("Rewound by " + String(close_ticks) + " ticks", NetLogLevel::DEBUG);

        // Remove slack on the line, cooldown
        delay(WINCH_CALIBRATE_PRE_DELAY);
        calibrate();
        cooldown();
    }

    /**
     * Wait time after a sequence, to prevent an immediate secondary sequence while subjects clear the sensor zones.
     */
    void cooldown()
    {
        // Close-wait after a sequence has been executed
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
        netLog("Door: OPEN", NetLogLevel::DEBUG);
        seq_start_time = millis();
        resetCprTimer();
        resetMaxTps();
        uint32_t open_distance = settings.getDoorOpenDistance();
        require_extended_close = false;  // used if there is jam during open (probably caused by bad calibration)

#ifndef DOOR_SONAR_FALLBACK
        // if the sonar fails, we'll use the door close ticks as a time-based open sequence
        fallback_mode = !waitForSonarReady();
#else
        // sonar disabled, use fallback mode
        fallback_mode = true;
#endif
        auto open_ticks = settings.getDoorCloseTicks() * WINCH_FALLBACK_OPEN_COEFFICIENT;

        if (!fallback_mode && (sonar.getDistance() > 0 && sonar.getDistance() < open_distance)) {
            netLog("Door open-jam detected, skipping open sequence", NetLogLevel ::WARNING);
            return 0;
        }

        setMotor(true, WINCH_MAX_POWER);
        delay(10);

        while (true) {
            runTickCallback();
            calcMaxTps();
            if (fallback_mode) {
                // Legacy/fallback mode
                if (checkForOpenKill() || (int_cpr_ticks > open_ticks)) {
                    netLog("Open halted at " + String(int_cpr_ticks) + " ticks", NetLogLevel::DEBUG);
                    break;
                }
            } else {
                // Sonar apex detection
                sonar.process();
                if (checkForOpenKill() || (sonar.getDistance() > 0 && sonar.getDistance() < open_distance)) {
                    netLog("Open halted at " + String(sonar.getDistance()) + "mm", NetLogLevel::DEBUG);
                    break;
                }
            }
        }

        netLog("Open max TPS: " + String(getMaxTps()), NetLogLevel::INFO);
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
        resetMaxTps();

        setMotor(false, WINCH_MAX_POWER);
        delay(10);

        while (!checkForCloseKill(ticks)) {
            delay(10);
            runTickCallback();
            calcMaxTps();
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
        netLog("Door: CALIBRATE", NetLogLevel::DEBUG);
        auto start_time = millis();
        resetCprTimer();
        resetMaxTps();

        setMotor(true, WINCH_MAX_POWER);
        bool noTension = false;

        /**
         * Primary phase.
         *
         * Wind the winch in, note the max speed and when the speed drops, assume that speed drop is caused by tension
         * on the line. Halt primary phase when either tension is detected, or we time out.
         */
        while (true) {
            delay(10);
            auto max_tps = calcMaxTps();
            if (max_tps > 0 && (getTicksPerSecond() < (max_tps * WINCH_CALIBRATE_TENSION_COEFFICIENT))) {
                // Tension detected, drop out
                break;
            } else if (millis() - start_time > WINCH_CALIBRATE_TIMEOUT) {
                // Tension never detected, alert and drop out
                netLog("Calibrate timeout detected, aborting calibration", NetLogLevel::WARNING);
                noTension = true;
                stopMotor();

                if (error_cb != nullptr) {
                    error_cb(DoorErrorCode::CALIBRATE_TIMEOUT, error_cb_ctx);
                }

                break;
            }
        }

        // Prep for secondary phase
        auto calibrateTicks = int_cpr_ticks;
        stopMotor();
        delay(100);
        resetCprTimer();
        setMotor(false, WINCH_MAX_POWER);
        start_time = millis();

        /**
         * Secondary phase.
         *
         * In this phase we will roll-back just a tad, as detecting the tension will likely pull the door open a little
         * bit.
         *
         * If a timeout occurred, it was probably because the door was already under tension and this calibration
         * attempt has just made it worse. So if this has happened, we'll roll-back 150% to undo damage instead of
         * making it worse.
         */
        unsigned long thresholdTicks = noTension ? calibrateTicks * 1.5 : WINCH_CALIBRATE_ROLLBACK_TICKS;
        while (int_cpr_ticks < thresholdTicks) {
            delay(10);

            // This is unlikely, but always have a timeout just in case
            if (millis() - start_time > WINCH_CALIBRATE_TIMEOUT) {
                netLog("Calibrate rollback timeout detected", NetLogLevel::ERROR);
                stopMotor();

                if (error_cb != nullptr) {
                    error_cb(DoorErrorCode::CALIBRATE_TIMEOUT, error_cb_ctx);
                }

                return;
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
    double tps_peak = 0;

    winchErrorCallback error_cb = nullptr;
    void* error_cb_ctx = nullptr;

    doorInterruptCallback interrupt_cb = nullptr;
    void* interrupt_cb_ctx = nullptr;

    tickCallback tick_cb = nullptr;
    void* tick_cb_ctx = nullptr;

    netLogCallback net_log_cb = nullptr;
    void* net_log_cb_ctx = nullptr;

    bool fallback_mode = false;
    bool require_extended_close = false;

    /**
     * Resets and initialises tick-data so that ticksPerSecond may be accurately called.
     */
    void resetCprTimer()
    {
        int_cpr_ticks = 0;
        cpr_last_ticks = 0;
        cpr_last_time = millis();
    }

    double calcMaxTps()
    {
        auto tps = getTicksPerSecond();
        if (tps > tps_peak) {
            tps_peak = tps;
        }

        return tps_peak;
    }

    [[nodiscard]] double getMaxTps() const
    {
        return tps_peak;
    }

    void resetMaxTps()
    {
        tps_peak = 0;
    }

    /**
     * Returns the number of CPR ticks per second.
     *
     * Calculated by determining the number of ticks since this function was last called. Call resetCprTimer() before
     * recording tick-rate.
     */
    double getTicksPerSecond()
    {
        static double last_tps = 0;

        auto now = millis();
        if (now - cpr_last_time < 5) {
            // div by zero & micro-timing safe guard
            return last_tps;
        }

        double coefficient = (double)1000 / (double)(now - cpr_last_time);
        last_tps = (double)(int_cpr_ticks - cpr_last_ticks) * coefficient;

        cpr_last_time = now;
        cpr_last_ticks = int_cpr_ticks;

        return last_tps;
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
            netLog("Door open sequence max time exceeded", NetLogLevel::WARNING);
            stopMotor();
            if (error_cb != nullptr) {
                error_cb(DoorErrorCode::OPEN_TIMEOUT, error_cb_ctx);
            }
            return true;
        }

        // Check for motor stall
        if (run_time > MOTOR_CPR_WARMUP && (getMaxTps() > 0) &&
            (getTicksPerSecond() < (getMaxTps() * WINCH_STALL_COEFFICIENT))) {
            netLog("Door blocked while opening", NetLogLevel::WARNING);
            stopMotor();
            require_extended_close = true;
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
        if (run_time > MOTOR_CPR_WARMUP && (getMaxTps() > 0) &&
            (getTicksPerSecond() < (getMaxTps() * WINCH_STALL_COEFFICIENT))) {
            netLog("Winch jammed (close sequence!)", NetLogLevel ::WARNING);
            stopMotor();
            if (error_cb != nullptr) {
                error_cb(DoorErrorCode::JAMMED, error_cb_ctx);
            }
            return true;
        }

        // Exceeded sequence max time
        if (run_time > MAX_DOOR_SEQ_TIME) {
            netLog("Door close sequence max time exceeded", NetLogLevel::WARNING);
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

        netLog("Sonar not reporting data!", NetLogLevel ::ERROR);

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
