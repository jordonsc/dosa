#pragma once

#include <cstring>

#include "const.h"
#include "payload.h"

namespace dosa {
namespace messages {

enum class LogMessageLevel : uint8_t
{
    DEBUG = 10,      // debug information, normally ignored
    INFO = 20,       // device is providing non-important information
    STATUS = 30,     // device is reporting on its status
    WARNING = 40,    // device has encountered a potential problem
    ERROR = 50,      // device has encountered a problem
    CRITICAL = 60,   // device is no longer able to function
    SECURITY = 100,  // a security alert has been triggered (eg attempt to activate a locked device)
};

/**
 * Network-level log message.
 *
 * Device may broadcast information to anyone listening, to allow inspection or debugging at a network level.
 */
class LogMessage : public Payload
{
   public:
    LogMessage(char const* log_msg, char const* dev_name, LogMessageLevel level = LogMessageLevel::INFO)
        : Payload(DOSA_COMMS_MSG_LOG, dev_name),
          payload(DOSA_COMMS_PAYLOAD_BASE_SIZE + strlen(log_msg) + 1)
    {
        buildBasePayload(payload);
        payload.set(DOSA_COMMS_PAYLOAD_BASE_SIZE, static_cast<uint8_t>(level));
        payload.set(DOSA_COMMS_PAYLOAD_BASE_SIZE + 1, log_msg, strlen(log_msg));
    }

    static LogMessage fromPacket(char const* packet, uint32_t size)
    {
        if (size < (DOSA_COMMS_PAYLOAD_BASE_SIZE + 1)) {
            // cannot log or throw an exception, so create a null Error packet
            return LogMessage("BAD_PACKET", bad_dev_name);
        }

        char nulled_message[size - DOSA_COMMS_PAYLOAD_BASE_SIZE];
        memcpy(nulled_message, packet + DOSA_COMMS_PAYLOAD_BASE_SIZE + 1, size - DOSA_COMMS_PAYLOAD_BASE_SIZE - 1);
        nulled_message[size - DOSA_COMMS_PAYLOAD_BASE_SIZE - 1] = 0;

        auto log_msg =
            LogMessage(nulled_message, packet + 7, *(LogMessageLevel*)(packet + DOSA_COMMS_PAYLOAD_BASE_SIZE));

        log_msg.msg_id = *(uint16_t*)packet;
        log_msg.buildBasePayload(log_msg.payload);

        return log_msg;
    }

    [[nodiscard]] char const* getPayload() const override
    {
        return payload.getPayload();
    }

    [[nodiscard]] uint16_t getPayloadSize() const override
    {
        return payload.getPayloadSize();
    }

    [[nodiscard]] LogMessageLevel getLogLevel() const
    {
        return *(LogMessageLevel*)payload.getPayload(DOSA_COMMS_PAYLOAD_BASE_SIZE);
    };

    [[nodiscard]] char const* getMessage() const
    {
        return payload.getPayload(DOSA_COMMS_PAYLOAD_BASE_SIZE + 1);
    }

    [[nodiscard]] uint16_t getMessageSize() const
    {
        return payload.getPayloadSize() - DOSA_COMMS_PAYLOAD_BASE_SIZE - 1;
    }

   private:
    VariablePayload payload;
};

}  // namespace messages
}  // namespace dosa
