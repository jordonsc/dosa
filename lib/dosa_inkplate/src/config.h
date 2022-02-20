#pragma once

#include <Arduino.h>
#include <comms.h>

namespace dosa {

struct InkplateConfig
{
    /**
     * Application name.
     *
     * Displayed during the boot sequence splash screen.
     */
    String app_name;

    /**
     * Path to a 200x200 logo, displayed during the splash screen.
     */
    String logo_filename;

    /**
     * Path to a text file containing the wifi access-point name and password.
     *
     * Should use a \n delimiter between AP and password.
     */
    String wifi_filename;

    /**
     * Path to the 128x128 error icon.
     */
    String error_filename;

    String wifi_ap;
    String wifi_pw;
    bool wait_for_serial = false;
    LogLevel log_level = LogLevel::INFO;
};

}  // namespace dosa
