#pragma once

#include <cstring>

#include "const.h"
#include "payload.h"

#define DOSA_COMMS_ACK_SIZE DOSA_COMMS_PAYLOAD_BASE_SIZE + 2
#define DOSA_COMMS_ACK_MSG_CODE "ack"

namespace dosa {
namespace messages {

class Ack : public Payload
{
   public:
    /**
     * Create an Ack from a message you want to acknowledge.
     */
    Ack(Payload const& msg, char const* dev_name)
        : Payload(DOSA_COMMS_ACK_MSG_CODE, dev_name),
          payload(DOSA_COMMS_ACK_SIZE),
          ack_msg_id(msg.getMessageId())
    {
        buildBasePayload(payload);
        payload.set(DOSA_COMMS_PAYLOAD_BASE_SIZE, ack_msg_id);
    }

    Ack(uint16_t ack_msg_id, char const* dev_name)
        : Payload(DOSA_COMMS_ACK_MSG_CODE, dev_name),
          payload(DOSA_COMMS_ACK_SIZE),
          ack_msg_id(ack_msg_id)
    {
        buildBasePayload(payload);
        payload.set(DOSA_COMMS_PAYLOAD_BASE_SIZE, ack_msg_id);
    }

    static Ack fromPacket(char const* packet, uint32_t size)
    {
        if (size != DOSA_COMMS_ACK_SIZE) {
            // cannot log or throw an exception, so create a null Ack packet
            return Ack(0, bad_dev_name);
        }

        uint16_t ack_msg_id;
        memcpy(&ack_msg_id, packet + DOSA_COMMS_PAYLOAD_BASE_SIZE, 2);

        auto ack = Ack(ack_msg_id, packet + 7);
        ack.msg_id = *(uint16_t*)packet;
        ack.buildBasePayload(ack.payload);

        return ack;
    }

    [[nodiscard]] char const* getPayload() const override
    {
        return payload.getPayload();
    }

    [[nodiscard]] uint16_t getPayloadSize() const override
    {
        return payload.getPayloadSize();
    }

    [[nodiscard]] uint16_t getAckMsgId() const
    {
        return ack_msg_id;
    }

    bool operator==(Ack const& a)
    {
        return ack_msg_id == a.ack_msg_id && msg_id == a.msg_id && strncmp(name, a.name, 20) == 0;
    }

    bool operator==(Ack const* a)
    {
        return operator==(*a);
    }

   private:
    VariablePayload payload;
    uint16_t ack_msg_id;
};

}  // namespace messages
}  // namespace dosa
