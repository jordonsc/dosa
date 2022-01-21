#pragma once

#include <dosa.h>

#include "const.h"
#include "sensor_container.h"

namespace dosa::sensor {

class SensorApp final : public dosa::App
{
   public:
    explicit SensorApp(Config const& config) : App(config) {}

    void init() override
    {
        App::init();
    }

    void loop() override
    {
        App::loop();

        // Check state of the IR grid
        checkIrGrid();
    }

   private:
    SensorContainer container;
    float grid[64] = {0};
    unsigned long last_fired = 0;

    void checkIrGrid()
    {
        if (millis() - ir_grid_last_update > IR_POLL) {
            ir_grid_last_update = millis();

            auto& ir = container.getIrGrid();
            auto& serial = container.getSerial();

            float new_grid[64] = {0};
            float max_delta = 0;
            float ttl_delta = 0;
            uint8_t changed = 0;

            for (unsigned row = 0; row < 8; ++row) {
                for (unsigned col = 0; col < 8; ++col) {
                    auto index = col + (row * 8);
                    new_grid[index] = ir.getPixelTemp(index);

                    float delta = abs(new_grid[index] - grid[index]);
                    if (delta >= SINGLE_DELTA_THRESHOLD) {
                        ++changed;
                        ttl_delta += delta;
                        if (delta > max_delta) {
                            max_delta = delta;
                        }
                    }
                }
            }

            triggerIf(max_delta, ttl_delta, changed);
            memcpy(grid, new_grid, sizeof(float) * 64);
        }
    }

    /**
     * Fire a trigger message if rules for comparing IR-grid deltas pass.
     *
     * Will also consider a cool-down before a second trigger.
     */
    bool triggerIf(float max_single_delta, float ttl_delta, uint8_t pixels_changed)
    {
        if (grid[0] == 0 || max_single_delta < SINGLE_DELTA_THRESHOLD || ttl_delta < TOTAL_DELTA_THRESHOLD ||
            pixels_changed < MIN_PIXELS_THRESHOLD || millis() - last_fired < REFIRE_DELAY) {
            return false;
        } else {
            container.getSerial().writeln("IR grid motion detected", LogLevel::DEBUG);
            last_fired = millis();
            container.getComms().dispatch(
                comms::multicastAddr,
                messages::Trigger(messages::TriggerDevice::IR_GRID, container.getSettings().getDeviceNameBytes()),
                true);
            return true;
        }
    }

    void onWifiConnect() override
    {
        App::onWifiConnect();

        // Bind a UDP port, we'll send triggers from this port and receive ack's here, too
        if (!container.getComms().bind(comms::udpPort)) {
            container.getSerial().writeln("Failed to bind UDP", LogLevel::ERROR);
        }
    }

    unsigned long ir_grid_last_update = 0;  // Last time we polled the sensor

    Container& getContainer() override
    {
        return container;
    }
};

}  // namespace dosa::sensor
