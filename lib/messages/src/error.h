#pragma once

#include <cstring>

#include "payload.h"

#define DOSA_COMMS_ERR_MSG_CODE "err"

namespace dosa::messages {

class Error : public Payload
{
   public:
    /**
     * Create an Error alert.
     */
    Error(char const* err_msg, char const* dev_name) : Payload(DOSA_COMMS_ERR_MSG_CODE, dev_name)
    {
        payload_size = DOSA_COMMS_PAYLOAD_BASE_SIZE + strlen(err_msg);
        payload = new char[payload_size];
        buildBasePayload(payload, payload_size);
        std::memcpy(payload + DOSA_COMMS_PAYLOAD_BASE_SIZE, err_msg, strlen(err_msg));
    }

    virtual ~Error()
    {
        delete payload;
    }

    static Error fromPacket(char const* packet, uint32_t size)
    {
        if (size < DOSA_COMMS_PAYLOAD_BASE_SIZE) {
            // cannot log or throw an exception, so create a null Error packet
            return Error("BAD_PACKET", bad_dev_name);
        }

        auto err = Error(packet + DOSA_COMMS_PAYLOAD_BASE_SIZE, packet + 7);
        err.msg_id = *(uint16_t*)packet;
        err.buildBasePayload(err.payload, size);

        return err;
    }

    [[nodiscard]] char const* getPayload() const override
    {
        return payload;
    }

    [[nodiscard]] uint16_t getPayloadSize() const override
    {
        return payload_size;
    }

    [[nodiscard]] char const* getErrorMessage() const
    {
        return payload + DOSA_COMMS_PAYLOAD_BASE_SIZE;
    }

    [[nodiscard]] uint16_t getErrorMessageSize() const
    {
        return payload_size - DOSA_COMMS_PAYLOAD_BASE_SIZE;
    }

   private:
    uint16_t payload_size;
    char* payload;
};

}  // namespace dosa::messages
