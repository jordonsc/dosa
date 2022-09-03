#pragma once

#include <cstring>

#include "const.h"
#include "payload.h"

namespace dosa {
namespace messages {

/**
 * Status report message.
 *
 * Variable payload status message.
 */
class StatusMessage : public Payload
{
   public:
    StatusMessage(uint16_t status_format, char const* status_payload, uint16_t status_size, char const* dev_name)
        : Payload(DOSA_COMMS_MSG_STATUS, dev_name),
          payload(DOSA_COMMS_PAYLOAD_BASE_SIZE + status_size + 2),
          status_format(status_format),
          status_size(status_size)
    {
        buildBasePayload(payload);
        payload.set(DOSA_COMMS_PAYLOAD_BASE_SIZE, status_format);
        payload.set(DOSA_COMMS_PAYLOAD_BASE_SIZE + 2, status_payload, status_size);
    }

    static StatusMessage fromPacket(char const* packet, uint32_t size)
    {
        if (size < (DOSA_COMMS_PAYLOAD_BASE_SIZE + 2)) {
            // cannot log or throw an exception, so create a null Error packet
            return {0, "BAD_PACKET", 10, bad_dev_name};
        }

        uint16_t fmt;
        memcpy(&fmt, packet + DOSA_COMMS_PAYLOAD_BASE_SIZE, 2);

        auto stat = StatusMessage(
            fmt,
            packet + DOSA_COMMS_PAYLOAD_BASE_SIZE + 2,
            size - DOSA_COMMS_PAYLOAD_BASE_SIZE - 2,
            packet + 7);

        stat.buildBasePayload(stat.payload);

        return stat;
    }

    [[nodiscard]] char const* getPayload() const override
    {
        return payload.getPayload();
    }

    [[nodiscard]] uint16_t getPayloadSize() const override
    {
        return payload.getPayloadSize();
    }

    [[nodiscard]] uint16_t getStatusFormat() const
    {
        return status_format;
    }

    [[nodiscard]] uint16_t getStatusSize() const
    {
        return status_size;
    }

    [[nodiscard]] char const* getStatusMessage() const
    {
        return payload.getPayload(DOSA_COMMS_PAYLOAD_BASE_SIZE + 2);
    }

   private:
    VariablePayload payload;
    uint16_t status_format;
    uint16_t status_size;
};

}  // namespace messages
}  // namespace dosa
