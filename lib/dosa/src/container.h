#pragma once

#include "bt.h"
#include "lights.h"
#include "pool.h"
#include "serial.h"

namespace dosa {

class Container
{
   public:
    Container(Container const&) = delete;
    void operator=(Container const&) = delete;

    static Container& getInstance()
    {
        static Container instance;
        return instance;
    }

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

    DevicePool& getDevicePool()
    {
        return device_pool;
    }

   protected:
    Container() : bluetooth(Bluetooth(&serial)), device_pool(DevicePool(&serial)) {}

    SerialComms serial;
    Lights lights;
    Bluetooth bluetooth;
    DevicePool device_pool;
};

}  // namespace dosa
