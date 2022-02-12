#pragma once

#include <dosa.h>

#include "const.h"
#include "sonar_container.h"

namespace dosa::sonar {

class SonarApp final : public dosa::App
{
   public:
    explicit SonarApp(Config const& config) : App(config) {}

    void init() override
    {
        App::init();
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

    void checkSonar()
    {
        if (!container.getSonar().process()) {
            return;
        }

        auto distance = container.getSonar().getDistance();

        // Zero distance implies the sensor didn't receive a bounce-back (beyond range)
        if ((millis() - last_fired > REFIRE_DELAY) && (distance > 0) &&
            (distance < last_distance * DOSA_SONAR_TRIGGER_THRESHOLD || last_distance == 0)) {
            sendTrigger(last_distance, distance);
            last_fired = millis();
        }

        last_distance = distance;
    }

    void sendTrigger(uint16_t previous, uint16_t current)
    {
        // Trigger data will contain two 16-bit numbers, the previous and new distance measurements
        uint8_t map[64] = {0};
        memcpy(map, &previous, 2);
        memcpy(map + 2, &current, 2);

        logln("Sonar trigger: " + String(previous) + "mm -> " + String(current) + "mm");
        dispatchMessage(messages::Trigger(messages::TriggerDevice::SONAR, map, getDeviceNameBytes()), true);
    }

    Container& getContainer() override
    {
        return container;
    }
};

}  // namespace dosa::sonar
