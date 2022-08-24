#pragma once

#include <Arduino.h>
#include <dosa_messages.h>

namespace dosa {

class StatefulApplication
{
   protected:
    [[nodiscard]] messages::DeviceState getDeviceState() const
    {
        return device_state;
    }

    void setDeviceState(messages::DeviceState deviceState)
    {
        device_state = deviceState;
    }

    [[nodiscard]] bool isWarnState() const
    {
        return device_state >= messages::DeviceState::MINOR_FAULT;
    }

    [[nodiscard]] bool isErrorState() const
    {
        return device_state > messages::DeviceState::MAJOR_FAULT;
    }

   private:
    messages::DeviceState device_state = messages::DeviceState::OK;
};

}  // namespace dosa
