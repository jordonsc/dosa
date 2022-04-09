/**
 * DOSA Security Alarm
 * arduino:samd:nano_33_iot
 *
 * Creates visual alarm indicators when hearing security messages.
 */

#include <dosa_alarm.h>

dosa::AppBuilder<dosa::AlarmApp> builder;

/**
 * Arduino setup
 */
void setup()
{
    auto& cfg = builder.getConfig();
    cfg.app_name = "DOSA Alarm";
    cfg.short_name = "DOSA-A";
    cfg.bluetooth_appearance = 0x0541;
    cfg.device_type = dosa::messages::DeviceType::ALARM;

    builder.getApp().init();
}

/**
 * Arduino main loop
 */
void loop()
{
    builder.getApp().loop();
}
