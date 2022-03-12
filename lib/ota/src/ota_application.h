#pragma once

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <dosa.h>

namespace dosa {

#ifndef DOSA_OTA_STORAGE
#define DOSA_OTA_STORAGE "https://storage.googleapis.com/dosa-ota/"
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
     * Context forwarder for OTA update request messages.
     */
    static void otaMessageForwarder(messages::GenericMessage const& msg, comms::Node const& sender, void* context)
    {
        static_cast<OtaApplication*>(context)->onOtaRequest(msg, sender);
    }
};

}  // namespace dosa
