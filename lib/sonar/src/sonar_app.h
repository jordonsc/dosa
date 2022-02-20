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
    uint16_t last_distance = 1;
    unsigned long last_fired = 0;
    uint16_t trigger_count = 0;  // number of trigger

    void checkSonar()
    {
        if (!container.getSonar().process()) {
            return;
        }

        auto distance = container.getSonar().getDistance();

        // Zero distance implies the sensor didn't receive a bounce-back (beyond range)
        if ((millis() - last_fired > REFIRE_DELAY) && (distance > 0) &&
            (distance < last_distance * DOSA_SONAR_TRIGGER_THRESHOLD || last_distance == 0)) {
            // Sensor is reading a trigger, consider if we should de-noise or fire -
            ++trigger_count;

            if (trigger_count >= container.getSettings().getSonarTriggerThreshold()) {
                // Trigger-warn state surpassed, fire trigger message
                sendTrigger(last_distance, distance);
                last_fired = millis();
                last_distance = distance;
                trigger_count = 0;
            } else {
                // Trigger-warn state: count up reads to de-noise until we're sure this is a real trigger
                logln(
                    "Sonar warning (" + String(trigger_count) + "): " + String(last_distance) + "mm -> " +
                        String(distance) + "mm",
                    LogLevel::DEBUG);
            }
        } else {
            // Not in a warn/firing mode
            last_distance = distance;
            trigger_count = 0;
        }
    }

    void sendTrigger(uint16_t previous, uint16_t current)
    {
        // Trigger data will contain two 16-bit numbers, the previous and new distance measurements
        uint8_t map[64] = {0};
        memcpy(map, &previous, 2);
        memcpy(map + 2, &current, 2);

        logln("Sonar TRIGGER: " + String(previous) + "mm -> " + String(current) + "mm");
        dispatchMessage(messages::Trigger(messages::TriggerDevice::SONAR, map, getDeviceNameBytes()), true);
    }

    Container& getContainer() override
    {
        return container;
    }
};

}  // namespace sonar
}  // namespace dosa
