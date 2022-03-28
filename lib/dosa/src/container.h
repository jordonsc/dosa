#pragma once

#include <dosa_comms.h>

#include "bt.h"
#include "fram.h"
#include "lights.h"
#include "settings.h"
#include "stats.h"

namespace dosa {

class Container
{
   public:
    Container()
        : ram(&serial),
          bluetooth(&serial),
          wifi(&serial),
          comms(wifi, &serial),
          stats(comms, &serial),
          settings(ram, &serial)
    {}

    [[nodiscard]] SerialComms& getSerial()
    {
        return serial;
    }

    [[nodiscard]] SerialComms const& getSerial() const
    {
        return serial;
    }

    [[nodiscard]] Lights& getLights()
    {
        return lights;
    }

    [[nodiscard]] Lights const& getLights() const
    {
        return lights;
    }

    [[nodiscard]] Bluetooth& getBluetooth()
    {
        return bluetooth;
    }

    [[nodiscard]] Bluetooth const& getBluetooth() const
    {
        return bluetooth;
    }

    [[nodiscard]] Wifi& getWiFi()
    {
        return wifi;
    }

    [[nodiscard]] Wifi const& getWiFi() const
    {
        return wifi;
    }

    [[nodiscard]] Comms& getComms()
    {
        return comms;
    }

    [[nodiscard]] Comms const& getComms() const
    {
        return comms;
    }

    Stats& getStats()
    {
        return stats;
    }

    Stats const& getStats() const
    {
        return stats;
    }

    [[nodiscard]] Settings& getSettings()
    {
        return settings;
    }

    [[nodiscard]] Settings const& getSettings() const
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
    Stats stats;
    Settings settings;
};

}  // namespace dosa
