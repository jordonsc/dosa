#pragma once

#include <dosa_comms.h>

#include "bt.h"
#include "fram.h"
#include "lights.h"
#include "settings.h"

namespace dosa {

class Container
{
   public:
    Container() : ram(&serial), bluetooth(&serial), wifi(&serial), comms(wifi, &serial), settings(ram, &serial) {}

    [[nodiscard]] SerialComms& getSerial()
    {
        return serial;
    }

    [[nodiscard]] Lights& getLights()
    {
        return lights;
    }

    [[nodiscard]] Bluetooth& getBluetooth()
    {
        return bluetooth;
    }

    [[nodiscard]] Wifi& getWiFi()
    {
        return wifi;
    }

    [[nodiscard]] Comms& getComms()
    {
        return comms;
    }

    [[nodiscard]] Settings& getSettings()
    {
        return settings;
    }

   protected:
    SerialComms serial;
    Fram ram;
    Lights lights;
    Bluetooth bluetooth;
    Wifi wifi;
    Comms comms;
    Settings settings;
};

}  // namespace dosa
