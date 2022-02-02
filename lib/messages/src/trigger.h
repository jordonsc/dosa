#pragma once

#include <cstring>

#include "const.h"
#include "payload.h"

#define DOSA_COMMS_TRIGGER_SIZE DOSA_COMMS_PAYLOAD_BASE_SIZE + 65

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
    Trigger(TriggerDevice device, uint8_t const* map, char const* dev_name)
        : Payload(DOSA_COMMS_MSG_TRIGGER, dev_name),
          device(device)
    {
        buildBasePayload(payload, DOSA_COMMS_TRIGGER_SIZE);
        std::memcpy(payload + DOSA_COMMS_PAYLOAD_BASE_SIZE, &device, 1);
        if (map != nullptr) {
            std::memcpy(payload + DOSA_COMMS_PAYLOAD_BASE_SIZE + 1, map, 64);
        }
    }

    static Trigger fromPacket(char const* packet, uint32_t size)
    {
        if (size != DOSA_COMMS_TRIGGER_SIZE) {
            // cannot log or throw an exception, so create a null Trigger packet
            return Trigger(TriggerDevice::NONE, nullptr, bad_dev_name);
        }

        TriggerDevice d;
        memcpy(&d, packet + DOSA_COMMS_PAYLOAD_BASE_SIZE, 1);

        auto trg = Trigger(d, (uint8_t*)packet + DOSA_COMMS_PAYLOAD_BASE_SIZE + 1, packet + 7);
        trg.msg_id = *(uint16_t*)packet;
        trg.buildBasePayload(trg.payload, DOSA_COMMS_TRIGGER_SIZE);

        return trg;
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

    [[nodiscard]] uint8_t const* getSensorMap() const
    {
        return (uint8_t*)payload + DOSA_COMMS_PAYLOAD_BASE_SIZE + 1;
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
