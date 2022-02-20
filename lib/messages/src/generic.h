#pragma once

#include <string>

#include "const.h"
#include "payload.h"

namespace dosa {
namespace messages {

/**
 * Some messages just need a command code, without any additional information. These can use a single class.
 *
 * This class also permits the use of additional, unknown generic information. However it's preferable to use a
 * dedicated class for anything with ancillary information.
 *
 * See const.h for a list of generic command codes.
 */
class GenericMessage final : public Payload
{
   public:
    explicit GenericMessage(char const* packet, uint16_t size, char const* dev_name)
        : Payload(*(uint16_t*)packet, packet + 2, dev_name),
          payload(size, packet)
    {}

    explicit GenericMessage(uint16_t msg_id, char const* cmd_code, char const* dev_name)
        : Payload(msg_id, cmd_code, dev_name),
          payload(DOSA_COMMS_PAYLOAD_BASE_SIZE)
    {
        buildBasePayload(payload);
    }

    explicit GenericMessage(char const* cmd_code, char const* dev_name)
        : Payload(cmd_code, dev_name),
          payload(DOSA_COMMS_PAYLOAD_BASE_SIZE)
    {
        buildBasePayload(payload);
    }

    static GenericMessage fromPacket(char const* packet, uint32_t size)
    {
        if (size < DOSA_COMMS_PAYLOAD_BASE_SIZE) {
            // cannot log or throw an exception, so create a null packet
            return GenericMessage(0, bad_cmd_code, bad_dev_name);
        }

        return GenericMessage(packet, size, packet + 7);
    }

    [[nodiscard]] char const* getPayload() const override
    {
        return payload.getPayload();
    }

    [[nodiscard]] uint16_t getPayloadSize() const override
    {
        return payload.getPayloadSize();
    }

    [[nodiscard]] char const* getMessage() const
    {
        return payload.getPayload(DOSA_COMMS_PAYLOAD_BASE_SIZE);
    }

    [[nodiscard]] uint16_t getMessageSize() const
    {
        return payload.getPayloadSize() - DOSA_COMMS_PAYLOAD_BASE_SIZE;
    }

   private:
    VariablePayload payload;
};

}  // namespace messages
}  // namespace dosa
