#pragma once

#include <cstring>

#include "const.h"
#include "payload.h"

namespace dosa::messages {

class Error : public Payload
{
   public:
    /**
     * Create an Error alert.
     */
    Error(char const* err_msg, char const* dev_name)
        : Payload(DOSA_COMMS_MSG_ERROR, dev_name),
          payload(DOSA_COMMS_PAYLOAD_BASE_SIZE + strlen(err_msg))
    {
        buildBasePayload(payload);
        payload.set(DOSA_COMMS_PAYLOAD_BASE_SIZE, err_msg, strlen(err_msg));
    }

    static Error fromPacket(char const* packet, uint32_t size)
    {
        if (size < DOSA_COMMS_PAYLOAD_BASE_SIZE) {
            // cannot log or throw an exception, so create a null Error packet
            return Error("BAD_PACKET", bad_dev_name);
        }

        auto err = Error(packet + DOSA_COMMS_PAYLOAD_BASE_SIZE, packet + 7);
        err.msg_id = *(uint16_t*)packet;
        err.buildBasePayload(err.payload);

        return err;
    }

    [[nodiscard]] char const* getPayload() const override
    {
        return payload.getPayload();
    }

    [[nodiscard]] uint16_t getPayloadSize() const override
    {
        return payload.getPayloadSize();
    }

    [[nodiscard]] char const* getErrorMessage() const
    {
        return payload.getPayload(DOSA_COMMS_PAYLOAD_BASE_SIZE);
    }

    [[nodiscard]] uint16_t getErrorMessageSize() const
    {
        return payload.getPayloadSize() - DOSA_COMMS_PAYLOAD_BASE_SIZE;
    }

   private:
    VariablePayload payload;
};

}  // namespace dosa::messages
