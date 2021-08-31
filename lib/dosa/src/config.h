#pragma once

#include <Arduino.h>
#include <ArduinoBLE.h>

#include "serial.h"

namespace dosa {

struct Config
{
    String app_name;
    String short_name;

    bool bluetooth_enabled = true;
    bool bluetooth_advertise = false;
    uint16_t bluetooth_appearance = 0;

    bool wifi_enabled = false;

    bool wait_for_serial = false;
    LogLevel log_level = LogLevel::INFO;
};

}  // namespace dosa