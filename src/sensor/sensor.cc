/**
 * DOSA Sensor Suite
 *
 * SATELLITE DEVICE
 * arduino:mbed_nano:nano33ble
 *
 * This application will monitor a series of hardware attached sensors and publish their status for any master devices
 * to take advantage of.
 */

#include <dosa_sensor.h>

dosa::AppBuilder<dosa::sensor::SensorApp> builder;

/**
 * Arduino setup
 */
void setup()
{
    auto& cfg = builder.getConfig();
    cfg.app_name = "DOSA Sensor Suite";
    cfg.short_name = "DOSA-S";
    cfg.bluetooth_appearance = 0x0541;
    cfg.bluetooth_advertise = true;

    builder.getApp().init();
}

/**
 * Arduino main loop
 */
void loop()
{
    builder.getApp().loop();
}
