#pragma once

#include <Arduino.h>
#include <ArduinoBLE.h>

#include "serial.h"

namespace dosa {

struct Config
{
    String app_name;
    String short_name;
    uint16_t bluetooth_appearance = 0;

    bool wait_for_serial = false;
    LogLevel log_level = LogLevel::INFO;
};

}  // namespace dosa
