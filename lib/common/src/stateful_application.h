#pragma once

#include <Arduino.h>
#include <messages.h>

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

    [[nodiscard]] bool isErrorState() const
    {
        return device_state >= messages::DeviceState::MINOR_FAULT;
    }

   private:
    messages::DeviceState device_state = messages::DeviceState::OK;
};

}  // namespace dosa
