/**
 * DOSA Active-IR Laser Sensor
 * arduino:samd:nano_33_iot
 *
 * Contains an active infrared ranging sensor that is used as a trip sensor.
 */

#include <dosa_laser.h>

dosa::AppBuilder<dosa::LaserApp> builder;

/**
 * Arduino setup
 */
void setup()
{
    auto& cfg = builder.getConfig();
    cfg.app_name = "DOSA Laser Sensor";
    cfg.short_name = "DOSA-L";
    cfg.bluetooth_appearance = 0x0541;
    cfg.device_type = dosa::messages::DeviceType::SENSOR_ACTIVE_IR;

    builder.getApp().init();
}

/**
 * Arduino main loop
 */
void loop()
{
    builder.getApp().loop();
}
