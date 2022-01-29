#pragma once

#include <string>

#include "payload.h"

namespace dosa::messages {

class GenericMessage final : public Payload
{
   public:
    explicit GenericMessage(char const* packet, uint16_t size, char const* dev_name)
        : Payload(*(uint16_t*)packet, packet + 2, dev_name),
          packet_size(size)
    {
        payload = new char[size];
        std::memcpy(payload, packet, size);
    }

    explicit GenericMessage(uint16_t msg_id, char const* cmd_code, char const* dev_name)
        : Payload(msg_id, cmd_code, dev_name),
          packet_size(DOSA_COMMS_PAYLOAD_BASE_SIZE)
    {
        payload = new char[DOSA_COMMS_PAYLOAD_BASE_SIZE];
        buildBasePayload(payload, DOSA_COMMS_PAYLOAD_BASE_SIZE);
    }

    explicit GenericMessage(char const* cmd_code, char const* dev_name)
        : Payload(cmd_code, dev_name),
          packet_size(DOSA_COMMS_PAYLOAD_BASE_SIZE)
    {
        payload = new char[DOSA_COMMS_PAYLOAD_BASE_SIZE];
        buildBasePayload(payload, DOSA_COMMS_PAYLOAD_BASE_SIZE);
    }

    virtual ~GenericMessage()
    {
        delete payload;
    }

    static GenericMessage fromPacket(char const* packet, uint32_t size)
    {
        if (size < DOSA_COMMS_PAYLOAD_BASE_SIZE) {
            // cannot log or throw an exception, so create a null packet
            return GenericMessage(0, bad_cmd_code, bad_dev_name);
        }

        uint16_t ack_msg_id;
        memcpy(&ack_msg_id, packet + DOSA_COMMS_PAYLOAD_BASE_SIZE, 2);
        return GenericMessage(packet, size, packet + 7);
    }

    [[nodiscard]] char const* getPayload() const override
    {
        return payload;
    }

    [[nodiscard]] uint16_t getPayloadSize() const override
    {
        return packet_size;
    }

    [[nodiscard]] char const* getMessage() const
    {
        return payload + DOSA_COMMS_PAYLOAD_BASE_SIZE;
    }

    [[nodiscard]] uint16_t getMessageSize() const
    {
        return packet_size - DOSA_COMMS_PAYLOAD_BASE_SIZE;
    }

   private:
    uint16_t const packet_size;
    char* payload = nullptr;
};

}  // namespace dosa::messages
