#pragma once

#include <dosa_ota.h>

#include "const.h"
#include "pir_container.h"

namespace dosa {

class PirApp final : public dosa::OtaApplication
{
   public:
    using dosa::OtaApplication::OtaApplication;

    void init() override
    {
        OtaApplication::init();
    }

    void loop() override
    {
        OtaApplication::loop();

        // Check state of the IR grid
        checkIrGrid();
    }

   private:
    PirContainer container;

    float grid[64] = {0};
    unsigned long last_fired = 0;
    unsigned long ir_grid_last_update = 0;  // Last time we polled the sensor

    void checkIrGrid()
    {
        if (millis() - ir_grid_last_update > IR_POLL) {
            ir_grid_last_update = millis();

            auto& ir = container.getIrGrid();
            auto& serial = container.getSerial();
            auto& settings = container.getSettings();

            float new_grid[64] = {0};
            float ttl_delta = 0;
            uint8_t changed = 0;
            uint8_t map[64] = {0};

            for (unsigned row = 0; row < 8; ++row) {
                for (unsigned col = 0; col < 8; ++col) {
                    auto index = col + (row * 8);
                    new_grid[index] = ir.getPixelTemp(index);

                    float delta = abs(new_grid[index] - grid[index]);
                    if (delta >= settings.getPirPixelDelta()) {
                        ++changed;
                        ttl_delta += delta;
                        map[index] = delta * 10;  // NB: truncated precision
                    } else {
                        map[index] = 0;
                    }
                }
            }

            triggerIf(ttl_delta, changed, map);
            memcpy(grid, new_grid, sizeof(float) * 64);
        }
    }

    /**
     * Fire a trigger message if rules for comparing IR-grid deltas pass.
     *
     * Will also consider a cool-down before a second trigger.
     */
    bool triggerIf(float ttl_delta, uint8_t pixels_changed, uint8_t const* map)
    {
        auto& settings = container.getSettings();

        if (grid[0] == 0 || ttl_delta < settings.getPirTotalDelta() ||
            pixels_changed < settings.getPirMinPixels() || millis() - last_fired < REFIRE_DELAY) {
            return false;
        }

        logln("IR grid motion detected");
        last_fired = millis();

        switch (getLockState()) {
            case LockState::UNLOCKED:
                // Normal mode: dispatch trigger
                logln("IR grid motion detected");
                dispatchMessage(
                    messages::Trigger(messages::TriggerDevice::SENSOR_GRID, map, getDeviceNameBytes()),
                    true);
                getStats().count(stats::trigger);
                break;
            default:
            case LockState::LOCKED:
                // Standard lock, ignore
                logln("Ignoring trip: locked");
                getStats().count(stats::sec_locked);
                break;
            case LockState::ALERT:
                // Security mode: dispatch sec alert
                getStats().count(stats::sec_alert);
                secAlert(SecurityLevel::ALERT);
                break;
            case LockState::BREACH:
                // Security mode: dispatch sec alert
                getStats().count(stats::sec_breached);
                secAlert(SecurityLevel::BREACH);
                break;
        }
        return true;
    }

    void onDebugRequest(messages::GenericMessage const& msg, comms::Node const& sender) override
    {
        if (msg_cache.validate(sender, msg.getMessageId())) {
            return;
        }

        App::onDebugRequest(msg, sender);
        netLog("Min pixels: " + String(getContainer().getSettings().getPirMinPixels()), sender);
        netLog("Per-pixel delta: " + String(getContainer().getSettings().getPirPixelDelta()), sender);
        netLog("Aggregate delta: " + String(getContainer().getSettings().getPirTotalDelta()), sender);
    }

    Container& getContainer() override
    {
        return container;
    }

    Container const& getContainer() const override
    {
        return container;
    }
};

}  // namespace dosa
