#pragma once

#include <cstring>

#include "const.h"
#include "payload.h"

#define DOSA_COMMS_ALT_SIZE DOSA_COMMS_PAYLOAD_BASE_SIZE + 2

namespace dosa {
namespace messages {

class Alt : public Payload
{
   public:
    Alt(uint16_t c, char const* dev_name) : Payload(DOSA_COMMS_MSG_ALT, dev_name), code(c)
    {
        buildBasePayload(payload, DOSA_COMMS_ALT_SIZE);
        std::memcpy(payload + DOSA_COMMS_PAYLOAD_BASE_SIZE, &code, sizeof(code));
    }

    static Alt fromPacket(char const* packet, uint32_t size)
    {
        if (size != DOSA_COMMS_ALT_SIZE) {
            // cannot log or throw an exception, so create a null Alt packet
            return {0, bad_dev_name};
        }

        uint16_t code;
        memcpy(&code, (uint8_t*)packet + DOSA_COMMS_PAYLOAD_BASE_SIZE, sizeof(code));

        auto alt = Alt(code, packet + 7);
        alt.msg_id = *(uint16_t*)packet;
        alt.buildBasePayload(alt.payload, DOSA_COMMS_ALT_SIZE);

        return alt;
    }

    [[nodiscard]] uint16_t getCode() const
    {
        return code;
    }

    [[nodiscard]] char const* getPayload() const override
    {
        return payload;
    }

    [[nodiscard]] uint16_t getPayloadSize() const override
    {
        return DOSA_COMMS_ALT_SIZE;
    }

    bool operator==(Alt const& a)
    {
        return code == a.code && msg_id == a.msg_id && strncmp(name, a.name, 20) == 0;
    }

    bool operator==(Alt const* a)
    {
        return operator==(*a);
    }

   private:
    uint16_t code;
    char payload[DOSA_COMMS_ALT_SIZE] = {0};
};

}  // namespace messages
}  // namespace dosa
