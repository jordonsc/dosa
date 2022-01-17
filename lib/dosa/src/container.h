#pragma once

#include "bt.h"
#include "fram.h"
#include "lights.h"
#include "serial.h"
#include "wifi.h"

namespace dosa {

class Container
{
   public:
    Container() : ram(&serial), bluetooth(&serial), wifi(&serial) {}

    [[nodiscard]] SerialComms& getSerial()
    {
        return serial;
    }

    [[nodiscard]] Fram& getFram()
    {
        return ram;
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

   protected:
    SerialComms serial;
    Fram ram;
    Lights lights;
    Bluetooth bluetooth;
    Wifi wifi;
};

}  // namespace dosa
