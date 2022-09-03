#pragma once

#include "const.h"

namespace dosa {
namespace messages {

enum class DeviceType : uint8_t
{
    UNSPECIFIED = 0,  //
    MONITOR = 1,      // Monitoring device
    UTILITY = 2,      // Utility application (such as config script)
    ALARM = 3,        // Security alarm (responds to security events)

    // Photonic sensors
    SENSOR_PIR = 10,        // Passive infrared (incl. IR grid)
    SENSOR_ACTIVE_IR = 11,  // Active infrared
    SENSOR_OPTICAL = 12,    // Optical camera

    // Sonic sensors
    SENSOR_SONAR = 20,  // Ultrasonic ranging trip sensor

    // Hydration sensors
    // Reserved: 30-39

    // Tactile sensors
    BUTTON = 40,  // Physical push-button
    TOGGLE = 41,  // Physical toggle-switch

    // Action devices
    POWER_TOGGLE = 110,  // Power toggle switch
    MOTOR_WINCH = 112,   // Motorised winch
    LIGHT = 113,         // Light controller

    // Power devices
    POWER_GRID = 120,     // Multi-component power grid
    BATTERY = 121,        // Battery
    SOLAR_PANEL = 122,    // PV
    POWER_MONITOR = 123,  // Power meter
};

enum class DeviceState : uint8_t
{
    OK = 0,       // Device is online, but not doing anything
    WORKING = 1,  // Device is online and actively performing a primary function
    TRIGGER = 2,  // Device has been triggered and in cool-down (sensor trip, etc)

    MINOR_FAULT = 10,  // A minor issue has been detected
    MAJOR_FAULT = 11,  // A serious issue has been detected; primary function impaired
    CRITICAL = 12,     // Device can no longer perform its primary function

    // Device itself should not declare its own state with below codes
    NOT_RESPONDING = 20,  // Device not responding to pings or commands
    UNKNOWN = 255,
};

}  // namespace messages
}  // namespace dosa
