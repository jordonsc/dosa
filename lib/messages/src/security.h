#pragma once

#include "const.h"
#include "payload.h"

#define DOSA_COMMS_SEC_SIZE DOSA_COMMS_PAYLOAD_BASE_SIZE + 1
#define DOSA_COMMS_SEC_MSG_CODE "sec"

namespace dosa {
namespace messages {

class Security : public Payload
{
   public:
    Security(SecurityLevel lvl, char const* dev_name)
        : Payload(DOSA_COMMS_SEC_MSG_CODE, dev_name),
          payload(DOSA_COMMS_SEC_SIZE),
          sec_level(lvl)
    {
        buildBasePayload(payload);
        payload.set(DOSA_COMMS_PAYLOAD_BASE_SIZE, static_cast<uint8_t>(lvl));
    }

    static Security fromPacket(char const* packet, uint32_t size)
    {
        if (size != DOSA_COMMS_SEC_SIZE) {
            // cannot log or throw an exception, so create a null Ack packet
            return Security(SecurityLevel::TAMPER, bad_dev_name);
        }

        SecurityLevel lvl;
        memcpy(&lvl, packet + DOSA_COMMS_PAYLOAD_BASE_SIZE, 1);

        auto sec = Security(lvl, packet + 7);
        sec.msg_id = *(uint16_t*)packet;
        sec.buildBasePayload(sec.payload);

        return sec;
    }

    [[nodiscard]] char const* getPayload() const override
    {
        return payload.getPayload();
    }

    [[nodiscard]] uint16_t getPayloadSize() const override
    {
        return payload.getPayloadSize();
    }

    [[nodiscard]] SecurityLevel getSecurityLevel() const
    {
        return sec_level;
    }

    bool operator==(Security const& a)
    {
        return sec_level == a.sec_level && msg_id == a.msg_id && strncmp(name, a.name, 20) == 0;
    }

    bool operator==(Security const* a)
    {
        return operator==(*a);
    }

   private:
    VariablePayload payload;
    SecurityLevel sec_level;
};

}  // namespace messages
}  // namespace dosa
