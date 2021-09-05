/**
 * DOSA Door Driver
 *
 * MASTER DEVICE
 * arduino:samd:nano_33_iot
 *
 * This programme will operate a powered winch triggered by either button press or a signal from a satellite device.
 */

#include <Arduino.h>
#include <dosa_door.h>

dosa::AppBuilder<dosa::door::DoorApp> builder;

/**
 * Arduino setup
 */
void setup()
{
    auto& cfg = builder.getConfig();
    cfg.app_name = "DOSA Door Controller";
    cfg.short_name = "DOSA-D";
    cfg.bluetooth_appearance = 0x0741;
    cfg.bluetooth_advertise = false;

    builder.getApp().init();
}

/**
 * Arduino main loop
 */
void loop()
{
    builder.getApp().loop();
}
