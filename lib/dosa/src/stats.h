#pragma once

#include <Statsd.h>
#include <dosa_messages.h>

#include "settings.h"

namespace dosa {

class StatsApplication
{
   protected:
    [[nodiscard]] virtual Container& getContainer() = 0;
    [[nodiscard]] virtual Container const& getContainer() const = 0;

    void setStatsServer(String const& server_addr, uint16_t server_port)
    {
        clearStatsServer();

        if (server_addr.length() > 0) {
            statsd = new Statsd(getContainer().getWiFi().getUdp(), server_addr, server_port);
        }
    }

    void clearStatsServer()
    {
        if (hasStatsServer()) {
            delete statsd;
        }
    }

    bool hasStatsServer() const
    {
        return statsd != nullptr;
    }

    Statsd& getStats()
    {
        if (!hasStatsServer()) {
            // Create a fake local statsd server to prevent crash
            statsd = new Statsd(getContainer().getWiFi().getUdp(), "127.0.0.1", 8125);
        }

        return *statsd;
    }

   private:
    Statsd* statsd;
};

}  // namespace dosa
