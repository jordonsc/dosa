#pragma once

#include <dosa.h>

#include "const.h"
#include "sonar_container.h"

namespace dosa {
namespace sonar {

class SonarApp final : public dosa::App
{
   public:
    explicit SonarApp(Config const& config) : App(config) {}

    void init() override
    {
        App::init();
        logln("Trigger threshold: " + String(container.getSettings().getSonarTriggerThreshold())), LogLevel::DEBUG;
    }

    void loop() override
    {
        App::loop();
        checkSonar();
    }

   private:
    SonarContainer container;
    uint16_t calibrated_distance = 1;
    unsigned long last_fired = 0;

    /**
     * Reads the sonar and will either auto-calibrate or fire a trigger.
     */
    void checkSonar()
    {
        static uint16_t trigger_count = 0;      // number of triggers (distance < last_distance)
        static uint16_t calibration_count = 0;  // number of calibrations (distance > last_distance)

        if (!container.getSonar().process()) {
            return;
        }

        if (millis() - last_fired < REFIRE_DELAY) {
            return;
        }

        // Zero distance implies the sensor didn't receive a bounce-back (beyond range)
        auto distance = container.getSonar().getDistance();

        if ((distance > 0) &&
            (distance < calibrated_distance * DOSA_SONAR_TRIGGER_THRESHOLD || calibrated_distance == 0)) {
            calibration_count = 0;
            considerTrigger(distance, trigger_count);
        } else {
            trigger_count = 0;
            calibrateSensor(distance, calibration_count);
        }
    }

    /**
     * Trigger mode.
     *
     * Sensor is reading a trigger state (distance < calibrated threshold), consider if we should wait for more
     * positives or actually fire.
     */
    void considerTrigger(uint16_t distance, uint16_t& trigger_count)
    {
        ++trigger_count;

        if (trigger_count >= container.getSettings().getSonarTriggerThreshold()) {
            // Trigger-warn state surpassed, fire trigger message
            setDeviceState(messages::DeviceState::WORKING);
            sendTrigger(calibrated_distance, distance);
            last_fired = millis();
            calibrated_distance = distance;
            trigger_count = 0;
        } else {
            // Trigger-warn state: count up reads to de-noise until we're sure this is a real trigger
            logln(
                "Sonar warning (" + String(trigger_count) + "): " + String(calibrated_distance) + "mm -> " +
                    String(distance) + "mm",
                LogLevel::DEBUG);
        }
    }

    /**
     * Calibration mode.
     *
     * To ensure an errant longer-distance isn't recorded, we'll also apply a threshold before increasing the
     * current distance.
     */
    void calibrateSensor(uint16_t distance, uint16_t& calibration_count)
    {
        if (calibrated_distance == distance) {
            // Measurement is bang on, do nothing
            calibration_count = 0;
            return;
        } else if (calibrated_distance == 0 && distance > 0) {
            // But we'll skip the threshold if we're setting a distance from an unknown/infinite value
            calibration_count = 0;
            calibrated_distance = distance;
            return;
        } else if (distance == 0 && calibrated_distance > 0) {
            // Suggesting a new distance of infinite
            ++calibration_count;
        } else if (distance > calibrated_distance) {
            // Distance has increased from last calibrated marker
            ++calibration_count;
        }

        if (calibration_count >= container.getSettings().getSonarTriggerThreshold()) {
            // OK, really looks like the distance has increased, accept the new distance as our calibrated marker
            calibrated_distance = distance;
            calibration_count = 0;
            logln("Distance calibration set to " + String(calibrated_distance), LogLevel::DEBUG);

            if (calibrated_distance == 0) {
                // We'll consider an infinite distance to be a 'minor issue'
                setDeviceState(messages::DeviceState::MINOR_FAULT);
            } else {
                setDeviceState(messages::DeviceState::OK);
            }
        }
    }

    /**
     * Broadcasts a trigger message.
     */
    void sendTrigger(uint16_t previous, uint16_t current)
    {
        // Trigger data will contain two 16-bit numbers, the previous and new distance measurements
        uint8_t map[64] = {0};
        memcpy(map, &previous, 2);
        memcpy(map + 2, &current, 2);

        logln("Sonar TRIGGER: " + String(previous) + "mm -> " + String(current) + "mm");
        dispatchMessage(messages::Trigger(messages::TriggerDevice::SENSOR_RANGING, map, getDeviceNameBytes()), true);
    }

    Container& getContainer() override
    {
        return container;
    }
};

}  // namespace sonar
}  // namespace dosa
