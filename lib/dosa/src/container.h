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
    Container() : fram(&serial), bluetooth(&serial), wifi(&serial) {}

    SerialComms& getSerial()
    {
        return serial;
    }

    Fram& getFram()
    {
        return fram;
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
    Fram fram;
    Lights lights;
    Bluetooth bluetooth;
    Wifi wifi;
};

}  // namespace dosa
