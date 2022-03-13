#pragma once

#include <Arduino.h>
#include <ArduinoHttpClient.h>
#include <ArduinoOTA.h>
#include <dosa.h>

namespace dosa {

#ifndef DOSA_OTA_HOST
#define DOSA_OTA_HOST "storage.googleapis.com"
#define DOSA_OTA_PORT 443
#define DOSA_OTA_PATH "/dosa-ota/"
#endif

class OtaApplication : public App
{
   public:
    using App::App;

    void init() override
    {
        App::init();

        // Inbound UDP request to initiate an OTA update
        getContainer().getComms().newHandler<comms::StandardHandler<messages::GenericMessage>>(
            DOSA_COMMS_MSG_OTA,
            &otaMessageForwarder,
            this);
    }

    void loop() override
    {
        App::loop();
    }

   private:
    /**
     * OTA update requested, check for new firmware.
     */
    void onOtaRequest(messages::GenericMessage const& msg, comms::Node const& sender)
    {
        // Send reply ack
        getContainer().getComms().dispatch(sender, messages::Ack(msg, getDeviceNameBytes()));
        netLog("OTA update initiated by " + comms::ipToString(sender.ip));
    }

    /**
     * Makes an HTTP request to the OTA server and retrieves the latest version number for this application.
     */
    uint32_t getOtaVersion()
    {
        WiFiSSLClient wifi_client;
        HttpClient http_client(wifi_client, DOSA_OTA_HOST, DOSA_OTA_PORT);

        String path(DOSA_OTA_PATH);
        path += config.short_name + "/version";
        http_client.get(path);

        int status = http_client.responseStatusCode();
        if (status != 200) {
            http_client.stop();
            netLog(
                "OTA version check failed; bad response code from OTA server: " + String(status),
                NetLogLevel::ERROR);
            return 0;
        }

        auto version = http_client.readStringUntil('\n');
        netLog("OTA update for " + config.short_name + " at version " + version);
        return version.toInt();
    }

    /**
     * Context forwarder for OTA update request messages.
     */
    static void otaMessageForwarder(messages::GenericMessage const& msg, comms::Node const& sender, void* context)
    {
        static_cast<OtaApplication*>(context)->onOtaRequest(msg, sender);
    }
};

}  // namespace dosa
