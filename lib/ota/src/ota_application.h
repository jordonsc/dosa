#pragma once

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <HttpClient.h>
#include <dosa.h>

namespace dosa {

#ifndef DOSA_OTA_HOST
#define DOSA_OTA_HOST "storage.googleapis.com"
#define DOSA_OTA_PORT 80
#define DOSA_OTA_PATH "/dosa-ota/"
#define DOSA_OTA_UA "DOSA OTA/1.0"
#endif

#define DOSA_OTA_SIZE_MIN 50000
#define DOSA_OTA_SIZE_MAX 250000

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

        auto ota_version = getOtaVersion();
        if (ota_version == 0) {
            return;
        } else if (ota_version > DOSA_VERSION) {
            performOtaUpdate(ota_version);
        } else {
            netLog("No new updates to apply (current: " + String(DOSA_VERSION) + ", OTA: " + String(ota_version) + ")");
        }
    }

    /**
     * Makes an HTTP request to the OTA server and retrieves the latest version number for this application.
     */
    uint32_t getOtaVersion()
    {
        WiFiClient wifi_client;
        HttpClient http_client(wifi_client);

        String path(DOSA_OTA_PATH);
        path += config.short_name + "/version?v=" + String(random(100000));
        http_client.get(DOSA_OTA_HOST, DOSA_OTA_PORT, path.c_str(), DOSA_OTA_UA);

        auto status = http_client.responseStatusCode();
        if (status != 200) {
            http_client.stop();
            netLog("OTA version check failed: " + String(status), NetLogLevel::ERROR);
            return 0;
        }

        http_client.skipResponseHeaders();

        auto version = http_client.readStringUntil('\n');
        return version.toInt();
    }

    void performOtaUpdate(uint32_t version)
    {
        netLog("Performing OTA update for " + config.short_name + " v" + version + "..");

        WiFiClient wifi_client;
        HttpClient http_client(wifi_client);

        String path(DOSA_OTA_PATH);
        path += config.short_name + "/build-" + String(version) + ".bin";
        http_client.get(DOSA_OTA_HOST, DOSA_OTA_PORT, path.c_str(), DOSA_OTA_UA);

        auto status = http_client.responseStatusCode();
        if (status != 200) {
            http_client.stop();
            netLog("OTA update download failed: " + String(status), NetLogLevel::ERROR);
            return;
        }

        http_client.skipResponseHeaders();
        auto content_length = http_client.contentLength();

        if (content_length == 0) {
            netLog("Cannot update via OTA; null content-length", NetLogLevel::ERROR);
            return;
        } else if (content_length < DOSA_OTA_SIZE_MIN) {
            netLog("Cannot update via OTA; payload size too small", NetLogLevel::ERROR);
            return;
        } else if (content_length > DOSA_OTA_SIZE_MAX) {
            netLog("Cannot update via OTA; payload size too large", NetLogLevel::ERROR);
            return;
        }

        if (!InternalStorage.open(content_length)) {
            http_client.stop();
            netLog("Insufficient space for OTA update", NetLogLevel::ERROR);
            return;
        }

        // We can only write to internal storage 1-byte at a time, so download the OTA update and write as we go
        byte b;
        while (content_length > 0) {
            if (!http_client.readBytes(&b, 1)) {
                // Timeout occurred
                netLog("Timeout while downloading OTA update");
                return;
            }

            InternalStorage.write(b);
            --content_length;
        }

        InternalStorage.close();
        http_client.stop();

        netLog("Device applying OTA update");
        InternalStorage.apply();
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
