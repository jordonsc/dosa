/**
 * DOSA Door Driver
 * arduino:samd:nano_33_iot
 *
 * This programme will operate a powered winch triggered by either button press or a signal from a satellite device.
 */

#include <dosa_door.h>

dosa::AppBuilder<dosa::DoorApp> builder;

/**
 * Arduino setup
 */
void setup()
{
    auto& cfg = builder.getConfig();
    cfg.app_name = "DOSA Winch Driver";
    cfg.short_name = "DOSA-D";
    cfg.bluetooth_appearance = 0x0741;
    cfg.device_type = dosa::messages::DeviceType::MOTOR_WINCH;

    builder.getApp().init();
}

/**
 * Arduino main loop
 */
void loop()
{
    builder.getApp().loop();
}
