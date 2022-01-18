#pragma once

#include <cstring>

#include "payload.h"

#define DOSA_COMMS_ACK_SIZE DOSA_COMMS_PAYLOAD_BASE_SIZE + 2

namespace dosa::messages {

class Ack : public Payload
{
   public:
    explicit Ack(uint16_t ack_msg_id) : Payload("ack"), ack_msg_id(ack_msg_id)
    {
        buildBasePayload(payload, DOSA_COMMS_ACK_SIZE);
        std::memcpy(payload + DOSA_COMMS_PAYLOAD_BASE_SIZE, &ack_msg_id, 2);
    }

    static Ack fromPacket(char const* packet)
    {
        return Ack(*(uint16_t*)packet, *(uint16_t*)(packet + DOSA_COMMS_PAYLOAD_BASE_SIZE));
    }

    [[nodiscard]] char const* getPayload() const override
    {
        return payload;
    }

    [[nodiscard]] uint16_t getPayloadSize() const override
    {
        return DOSA_COMMS_ACK_SIZE;
    }

    [[nodiscard]] uint16_t getAckMsgId() const
    {
        return ack_msg_id;
    }

    bool operator==(Ack const& a)
    {
        return ack_msg_id == a.ack_msg_id && msg_id == a.msg_id;
    }

    bool operator==(Ack const* a)
    {
        return operator==(*a);
    }

   private:
    uint16_t ack_msg_id;
    char payload[DOSA_COMMS_ACK_SIZE] = {0};

    Ack(uint16_t msgId, uint16_t ack_msg_id) : Payload(msgId, "ack"), ack_msg_id(ack_msg_id)
    {
        buildBasePayload(payload, DOSA_COMMS_ACK_SIZE);
        std::memcpy(payload + DOSA_COMMS_PAYLOAD_BASE_SIZE, &ack_msg_id, 2);
    }
};

}  // namespace dosa::messages
