#pragma once

#include <dosa.h>

#include "sensor_container.h"

/**
 * Network setting - the number of times we'll send a UDP multicast message without receiving an ack in return before
 * giving up and assuming nobody is listening. Increase this number to deal with poor network transmission.
 */
#define MAX_ACK_RETRIES 5

/**
 * Polling time (in ms) to read and compare IR grid numbers. Increasing this number will normally result in larger
 * changes in the delta, thus increasing sensitivity. Changing this will potentially throw off other calibrations
 * dramatically.
 */
#define IR_POLL 500

/**
 * Temp change (in Celsius) before considering any single pixel as "changed". This is a de-noising threshold, increase
 * this number to reduce the amount of noise the algorithm is sensitive to.
 */
#define SINGLE_DELTA_THRESHOLD 1.5

/**
 * The total temperature delta across all pixels before firing a trigger. This is the primary sensitivity metric, it
 * is also filtered against noise by SINGLE_DELTA_THRESHOLD so it won't show a true full-grid delta.
 */
#define TOTAL_DELTA_THRESHOLD 25.0

/**
 * Minimum number of pixels that are considered 'changed' before we accept a trigger. Increase this to eliminate
 * single-pixel or edge anomalies.
 */
#define MIN_PIXELS_THRESHOLD 1

/**
 * Time (in ms) before firing a second trigger message.
 */
#define REFIRE_DELAY 3000

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
        stdLoop();

        // Check state of the IR grid
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

   private:
    SensorContainer container;
    float grid[64] = {0};
    unsigned long last_fired = 0;

    /**
     * Fire a trigger message if rules for comparing IR grid deltas pass.
     *
     * Will also consider a cool-down before a second trigger.
     */
    bool triggerIf(float max_single_delta, float ttl_delta, uint8_t pixels_changed)
    {
        if (grid[0] == 0 || max_single_delta < SINGLE_DELTA_THRESHOLD || ttl_delta < TOTAL_DELTA_THRESHOLD ||
            pixels_changed < MIN_PIXELS_THRESHOLD || millis() - last_fired < REFIRE_DELAY) {
            return false;
        } else {
            dispatchNotification();
            return true;
        }
    }

    /**
     * Send a notification via UDP multicast informing of sensor hit.
     *
     * This is a recursive function that will retry if we don't receive an ack back from a central device.
     */
    void dispatchNotification(uint8_t retries = 0)
    {
        auto& udp = container.getWiFi().getUdp();
        auto& serial = container.getSerial();

        if (retries == 0) {
            last_fired = millis();
            serial.writeln("IR trigger", dosa::LogLevel::INFO);
        }

        if (!container.getWiFi().isConnected()) {
            return;
        }

        if (udp.beginPacket(dosa::wifi::sensorBroadcastIp, dosa::wifi::sensorBroadcastPort) != 1) {
            serial.writeln("WARNING: unable to dispatch sensor trigger notification", dosa::LogLevel::WARNING);
            return;
        }

        String payload = "IR HIT";
        udp.write(payload.c_str());

        if (udp.endPacket() != 1) {
            serial.writeln("ERROR: unable to dispatch sensor trigger notification", dosa::LogLevel::ERROR);
            return;
        }

        // Wait for ack
        auto start = millis();
        char buffer[255];
        do {
            if (udp.parsePacket()) {
                auto data_size = min(udp.available(), 255);
                udp.read(buffer, data_size);
                buffer[data_size] = 0x0;
                auto incoming = String(buffer);
                if (incoming == "ack") {
                    serial.writeln(
                        "Trigger acknowledged by " + dosa::Wifi::ipToString(udp.remoteIP()),
                        dosa::LogLevel::INFO);
                    return;
                } else {
                    serial.writeln(
                        "Unexpected reply '" + incoming + "' from " + dosa::Wifi::ipToString(udp.remoteIP()),
                        dosa::LogLevel::WARNING);
                }
                break;
            }
        } while (millis() - start < 250);

        // Didn't get an ack in a timely manner, retry the notification
        if (retries < MAX_ACK_RETRIES) {
            dispatchNotification(retries + 1);
        } else {
            serial.writeln("WARNING: trigger has gone unacknowledged", dosa::LogLevel::WARNING);
        }
    }

    void onWifiConnect() override
    {
        App::onWifiConnect();
        container.getWiFi().getUdp().begin(random(1024, 65536));
    }

    unsigned long ir_grid_last_update = 0;  // Last time we polled the sensor

    Container& getContainer() override
    {
        return container;
    }
};

}  // namespace dosa::sensor
