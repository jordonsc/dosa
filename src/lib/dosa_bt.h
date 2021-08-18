/**
 * Bluetooth Comms.
 */

#pragma once

#include <ArduinoBLE.h>

#include "dosa_error.h"
#include "dosa_lights.h"

namespace dosa {

class Bluetooth
{
   public:
    static Bluetooth& getInstance()
    {
        static Bluetooth instance;
        return instance;
    }

    void setName(char const* name)
    {
        BLE.setLocalName(name);
    }

    void setAdvertise(bool advertise)
    {
        if (advertise) {
            BLE.advertise();
        } else {
            BLE.stopAdvertise();
        }
    }

    auto scanForService(char const* service)
    {
        BLE.scanForUuid(service);
        return BLE.available();
    }

   private:
    Bluetooth()
    {
        BLE.setDeviceName("DOSA Driver");
    }
};

}  // namespace dosa
