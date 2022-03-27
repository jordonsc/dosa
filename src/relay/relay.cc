/**
 * DOSA Relay Switch
 * arduino:samd:nano_33_iot
 *
 * Triggers toggle a relay, creating a power switch.
 */

#include <dosa_relay.h>

dosa::AppBuilder<dosa::RelayApp> builder;

/**
 * Arduino setup
 */
void setup()
{
    auto& cfg = builder.getConfig();
    cfg.app_name = "DOSA Relay";
    cfg.short_name = "DOSA-R";
    cfg.bluetooth_appearance = 0x0541;
    cfg.device_type = dosa::messages::DeviceType::POWER_TOGGLE;

    builder.getApp().init();
}

/**
 * Arduino main loop
 */
void loop()
{
    builder.getApp().loop();
}
