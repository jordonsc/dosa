#pragma once

#include <Arduino.h>

namespace dosa {

class InkplateConfig
{
   public:
    InkplateConfig() = default;

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

    String wifi_ap;
    String wifi_pw;
};

}  // namespace dosa
