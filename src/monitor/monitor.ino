/**
 * DOSA Display Monitor
 * Croduino_Boards:Inkplate:Inkplate6
 *
 * Uses an Inkplate display to show the status of all DOSA devices.
 */

#include <dosa_monitor.h>

dosa::InkAppBuilder<dosa::MonitorApp> builder;

/**
 * Arduino setup
 */
void setup()
{
    auto &cfg = builder.getConfig();
    cfg.app_name = String("DOSA Monitor");
    cfg.wifi_filename = dosa::config::wifi;
    cfg.logo_filename = dosa::images::logo;
    cfg.error_filename = dosa::images::error_inactive;
    cfg.log_level = dosa::LogLevel::DEBUG;
    cfg.wait_for_serial = true;

    builder.getApp().init();
}

/**
 * Arduino main loop
 */
void loop()
{
    builder.getApp().loop();
}
