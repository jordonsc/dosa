#pragma once

#include <dosa_comms.h>
#include <dosa_messages.h>

#include "settings.h"

namespace dosa {

class Stats : public virtual Loggable
{
   public:
    Stats(Comms& comms, SerialComms* s = nullptr) : Loggable(s), comms(comms) {}

    void setStatsServer(comms::Node server)
    {
        logln("Set stats server: " + comms::ipToString(server.ip) + ":" + String(server.port), LogLevel::DEBUG);
        stats_server = server;
    }

    void count(String const& key, uint32_t amount = 1) const
    {
        send(key + ":" + String(amount) + "|c");
    }

    void gauge(String const& key, uint32_t value) const
    {
        send(key + ":" + String(value) + "|g");
    }

    void timing(String const& key, uint32_t value) const
    {
        send(key + ":" + String(value) + "|ms");
    }

    String const& getTags() const
    {
        return tags;
    }

    void setTags(String const& value)
    {
        tags = value;
        cleanTags();
        logln("Set stats tags to: " + tags, LogLevel::DEBUG);
    }

   protected:
    Comms& comms;
    comms::Node stats_server = {IPAddress(), 0};
    String tags = "";

    void cleanTags()
    {
        tags.toLowerCase();
        for (size_t i = 0; i < tags.length(); ++i) {
            char c = tags.charAt(i);
            if (c != '_' && c != ':' && c != ',' && (tags.charAt(i) < 'a' || tags.charAt(i) > 'z') &&
                (tags.charAt(i) < '0' || tags.charAt(i) > '9')) {
                tags.setCharAt(i, '_');
            }
        }
    }

    void send(String payload) const
    {
        if (stats_server.port == 0) {
            return;
        }

        if (tags.length() > 0) {
            payload += "|#" + tags;
        }

        logln(
            "Send \"" + payload + "\" to " + comms::ipToString(stats_server.ip) + ":" + String(stats_server.port),
            LogLevel::DEBUG);
        if (!comms.dispatchRaw(stats_server, payload.c_str(), payload.length())) {
            logln("Stats dispatch failed!", LogLevel::ERROR);
        }
    }
};

}  // namespace dosa
