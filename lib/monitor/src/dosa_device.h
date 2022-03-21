#pragma once

#include <Arduino.h>
#include <dosa_messages.h>

#include <utility>

namespace dosa {

class DosaDevice
{
   public:
    DosaDevice()
        : device_name(String()),
          device_type(messages::DeviceType::UNSPECIFIED),
          device_state(messages::DeviceState::UNKNOWN),
          address({{0, 0, 0, 0}, 0}),
          state_last_updated(millis()),
          last_contact(millis())
    {}

    DosaDevice(
        String deviceName,
        messages::DeviceType deviceType,
        messages::DeviceState deviceState,
        comms::Node address)
        : device_name(std::move(deviceName)),
          device_type(deviceType),
          device_state(deviceState),
          address(std::move(address)),
          state_last_updated(millis()),
          last_contact(millis())
    {}

    static DosaDevice fromPong(messages::Pong const& pong, comms::Node const& sender)
    {
        return {dosa::stringFromBytes(pong.getDeviceName(), 20), pong.getDeviceType(), pong.getDeviceState(), sender};
    }

    [[nodiscard]] String const& getDeviceName() const
    {
        return device_name;
    }

    [[nodiscard]] messages::DeviceType getDeviceType() const
    {
        return device_type;
    }

    [[nodiscard]] messages::DeviceState getDeviceState() const
    {
        return device_state;
    }

    [[nodiscard]] comms::Node const& getAddress() const
    {
        return address;
    }

    void setDeviceName(String deviceName)
    {
        device_name = std::move(deviceName);
    }

    void setDeviceState(messages::DeviceState deviceState)
    {
        device_state = deviceState;
        state_last_updated = millis();
    }

    /**
     * Timestamp of when the device state was last updated.
     */
    [[nodiscard]] uint32_t getStateLastUpdated() const
    {
        return state_last_updated;
    }

    /**
     * Get the timestamp of the last known contact with this device.
     */
    [[nodiscard]] uint32_t getLastContact() const
    {
        return last_contact;
    }

    /**
     * Update the last contact time to now.
     */
    void reportContact()
    {
        last_contact = millis();
    }

    [[nodiscard]] bool operator==(DosaDevice const& d) const
    {
        return device_name == d.device_name && device_type == d.device_type && device_state == d.device_state &&
               address == d.address;
    }

    [[nodiscard]] bool operator!=(DosaDevice const& d) const
    {
        return !operator==(d);
    }

    static char const* typeToString(messages::DeviceType d)
    {
        switch (d) {
            default:
            case messages::DeviceType::UNSPECIFIED:
                return "Unknown";
            case messages::DeviceType::MONITOR:
                return "Monitor";
            case messages::DeviceType::UTILITY:
                return "Utility";
            case messages::DeviceType::SENSOR_PIR:
                return "PIR Sensor";
            case messages::DeviceType::SENSOR_ACTIVE_IR:
                return "IR Sensor";
            case messages::DeviceType::SENSOR_OPTICAL:
                return "Optical Sensor";
            case messages::DeviceType::SENSOR_SONAR:
                return "Sonar Sensor";
            case messages::DeviceType::BUTTON:
                return "Button";
            case messages::DeviceType::POWER_TOGGLE:
                return "Switch";
            case messages::DeviceType::MOTOR_WINCH:
                return "Motor Winch";
            case messages::DeviceType::LIGHT:
                return "Light Controller";
        }
    }

    [[nodiscard]] static char const* stateToString(messages::DeviceState d)
    {
        switch (d) {
            case messages::DeviceState::OK:
                return "OK";
            case messages::DeviceState::WORKING:
                return "Working";
            case messages::DeviceState::TRIGGER:
                return "Triggered";
            case messages::DeviceState::MINOR_FAULT:
                return "Minor Fault";
            case messages::DeviceState::MAJOR_FAULT:
                return "MAJOR FAULT";
            case messages::DeviceState::CRITICAL:
                return "CRITICAL";
            case messages::DeviceState::NOT_RESPONDING:
                return "NOT RESPONDING";
            default:
                return "Unknown";
        }
    }

   protected:
    String device_name;
    messages::DeviceType device_type;
    messages::DeviceState device_state;
    comms::Node address;
    uint32_t state_last_updated;
    uint32_t last_contact;
};

}  // namespace dosa
