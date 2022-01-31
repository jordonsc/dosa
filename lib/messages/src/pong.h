#pragma once

#include <cstring>

#include "health.h"
#include "payload.h"

#define DOSA_COMMS_PONG_SIZE DOSA_COMMS_PAYLOAD_BASE_SIZE + 2
#define DOSA_COMMS_PONG_MSG_CODE "pon"

namespace dosa::messages {

class Pong : public Payload
{
   public:
    Pong(DeviceType dt, DeviceState dh, char const* dev_name)
        : Payload(DOSA_COMMS_PONG_MSG_CODE, dev_name),
          device_type(dt),
          device_health(dh)
    {
        buildBasePayload(payload, DOSA_COMMS_PONG_SIZE);
        std::memcpy(payload + DOSA_COMMS_PAYLOAD_BASE_SIZE, &device_type, 1);
        std::memcpy(payload + DOSA_COMMS_PAYLOAD_BASE_SIZE + 1, &device_health, 1);
    }

    static Pong fromPacket(char const* packet, uint32_t size)
    {
        if (size != DOSA_COMMS_PONG_SIZE) {
            // cannot log or throw an exception, so create a null Pong packet
            return Pong(DeviceType::UNSPECIFIED, DeviceState::MAJOR_FAULT, bad_dev_name);
        }

        DeviceType dt;
        DeviceState dh;
        memcpy(&dt, packet + DOSA_COMMS_PAYLOAD_BASE_SIZE, 1);
        memcpy(&dh, packet + DOSA_COMMS_PAYLOAD_BASE_SIZE + 1, 1);

        auto pong = Pong(dt, dh, packet + 7);
        pong.buildBasePayload(pong.payload, DOSA_COMMS_PONG_SIZE);

        return pong;
    }

    [[nodiscard]] char const* getPayload() const override
    {
        return payload;
    }

    [[nodiscard]] uint16_t getPayloadSize() const override
    {
        return DOSA_COMMS_PONG_SIZE;
    }

   private:
    DeviceType device_type;
    DeviceState device_health;
    char payload[DOSA_COMMS_PONG_SIZE] = {0};
};

}  // namespace dosa::messages
