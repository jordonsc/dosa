#pragma once

#include "bt.h"
#include "lights.h"
#include "serial.h"
#include "wifi.h"

namespace dosa {

class Container
{
   public:
    Container() : bluetooth(&serial), wifi(&serial) {}

    SerialComms& getSerial()
    {
        return serial;
    }

    Lights& getLights()
    {
        return lights;
    }

    Bluetooth& getBluetooth()
    {
        return bluetooth;
    }

    Wifi& getWiFi()
    {
        return wifi;
    }

   protected:
    SerialComms serial;
    Lights lights;
    Bluetooth bluetooth;
    Wifi wifi;
};

}  // namespace dosa
