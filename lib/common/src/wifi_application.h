#pragma once

#include <Arduino.h>

namespace dosa {

class WifiApplication : public virtual Loggable
{
   protected:
    explicit WifiApplication(SerialComms* serial) : Loggable(serial), wifi(serial), comms(wifi, serial) {}

    Wifi wifi;
    Comms comms;

    void bindMulticast()
    {
        logln("Binding multicast group..");
        comms.bindMulticast(dosa::comms::multicastAddr);
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
