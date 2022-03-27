/**
 * DOSA Sensor Suite
 * arduino:samd:nano_33_iot
 *
 * Contains a sonar ranging sensor that is used as a trip sensor.
 */

#include <dosa_sonar.h>

dosa::AppBuilder<dosa::SonarApp> builder;

/**
 * Arduino setup
 */
void setup()
{
    auto& cfg = builder.getConfig();
    cfg.app_name = "DOSA Sonar Sensor";
    cfg.short_name = "DOSA-S";
    cfg.bluetooth_appearance = 0x0541;
    cfg.device_type = dosa::messages::DeviceType::SENSOR_SONAR;

    builder.getApp().init();
}

/**
 * Arduino main loop
 */
void loop()
{
    builder.getApp().loop();
}
