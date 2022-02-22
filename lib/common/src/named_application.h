#pragma once

#include <Arduino.h>

namespace dosa {

class NamedApplication
{
   protected:
    explicit NamedApplication(String deviceName) : device_name(std::move(deviceName))
    {
        updateDeviceNameBytes();
    }

    String const& getDeviceName() const
    {
        return device_name;
    }

    void setDeviceName(String const& deviceName)
    {
        device_name = deviceName;
        updateDeviceNameBytes();
    }

    char const* getDeviceNameBytes() const
    {
        return device_name_bytes;
    }

   private:
    String device_name;
    char device_name_bytes[20] = {0};

    /**
     * Rebuild the 20x char array for the device name.
     */
    void updateDeviceNameBytes()
    {
        memset(device_name_bytes, 0, 20);
        memcpy(device_name_bytes, device_name.c_str(), device_name.length());
    }
};

}  // namespace dosa
