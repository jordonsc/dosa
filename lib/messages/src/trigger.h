#pragma once

#include <cstring>

#include "payload.h"

#define DOSA_COMMS_TRIGGER_SIZE DOSA_COMMS_PAYLOAD_BASE_SIZE + 1
#define DOSA_COMMS_TRIGGER_MSG_CODE "trg"

namespace dosa::messages {

enum class TriggerDevice : uint8_t
{
    NONE = 0,  // for error conditions
    BUTTON = 1,
    PIR = 10,
    IR_GRID = 11,
};

class Trigger : public Payload
{
   public:
    Trigger(TriggerDevice device, char const* dev_name) : Payload(DOSA_COMMS_TRIGGER_MSG_CODE, dev_name), device(device)
    {
        buildBasePayload(payload, DOSA_COMMS_TRIGGER_SIZE);
        std::memcpy(payload + DOSA_COMMS_PAYLOAD_BASE_SIZE, &device, 1);
    }

    Trigger(uint16_t message_id, TriggerDevice device, char const* dev_name)
        : Payload(message_id, DOSA_COMMS_TRIGGER_MSG_CODE, dev_name),
          device(device)
    {
        buildBasePayload(payload, DOSA_COMMS_TRIGGER_SIZE);
        std::memcpy(payload + DOSA_COMMS_PAYLOAD_BASE_SIZE, &device, 1);
    }

    static Trigger fromPacket(char const* packet, uint32_t size)
    {
        if (size != DOSA_COMMS_ACK_SIZE) {
            // cannot log or throw an exception, so create a null Trigger packet
            return Trigger(0, TriggerDevice::NONE, bad_dev_name);
        }

        TriggerDevice d;
        memcpy(&d, packet + DOSA_COMMS_PAYLOAD_BASE_SIZE, 1);
        return Trigger(*(uint16_t*)packet, d, packet + 7);
    }

    [[nodiscard]] char const* getPayload() const override
    {
        return payload;
    }

    [[nodiscard]] uint16_t getPayloadSize() const override
    {
        return DOSA_COMMS_TRIGGER_SIZE;
    }

    [[nodiscard]] TriggerDevice getDeviceType() const
    {
        return device;
    }

    bool operator==(Trigger const& a)
    {
        return device == a.device && msg_id == a.msg_id && strncmp(name, a.name, 20) == 0;
    }

    bool operator==(Trigger const* a)
    {
        return operator==(*a);
    }

   private:
    TriggerDevice device;
    char payload[DOSA_COMMS_TRIGGER_SIZE] = {0};
};

}  // namespace dosa::messages
