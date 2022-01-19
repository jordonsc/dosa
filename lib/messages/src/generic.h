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

    virtual ~GenericMessage()
    {
        delete payload;
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
