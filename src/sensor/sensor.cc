/**
 * DOSA Motion Sensor
 * arduino:samd:nano_33_iot
 *
 * Contains an infrared grid that is used to passively detect motion.
 */

#include <dosa_sensor.h>

dosa::AppBuilder<dosa::sensor::SensorApp> builder;

/**
 * Arduino setup
 */
void setup()
{
    auto& cfg = builder.getConfig();
    cfg.app_name = "DOSA Motion Sensor";
    cfg.short_name = "DOSA-M";
    cfg.bluetooth_appearance = 0x0541;
    cfg.device_type = dosa::messages::DeviceType::SENSOR_MOTION;

    builder.getApp().init();
}

/**
 * Arduino main loop
 */
void loop()
{
    builder.getApp().loop();
}
