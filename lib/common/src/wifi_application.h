#pragma once

#include <Arduino.h>
#include <dosa_comms.h>

namespace dosa {

class WifiApplication
{
   protected:
    explicit WifiApplication(SerialComms* serial) : wifi(serial), comms(wifi, serial) {}

    Wifi wifi;
    Comms comms;

    bool bindMulticast()
    {
        return comms.bindMulticast(dosa::comms::multicastAddr);
    }

    Wifi& getWifi()
    {
        return wifi;
    }
    Comms& getComms()
    {
        return comms;
    }
};

}  // namespace dosa
