#pragma once

#include "const.h"

namespace dosa::messages {

enum class DeviceType : uint8_t
{
    UNSPECIFIED = 0,  // for error conditions only

    // Trigger devices
    SENSOR_MOTION = 10,
    SENSOR_SONAR = 11,
    BUTTON = 12,

    // Action devices
    SWITCH = 50,
    MOTOR_WINCH = 51,
};

enum class DeviceState : uint8_t
{
    OK = 0,
    WORKING = 1,

    MINOR_FAULT = 10,
    MAJOR_FAULT = 11,
    CRITICAL = 12,
};

}  // namespace dosa::messages