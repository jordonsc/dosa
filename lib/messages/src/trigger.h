#pragma once

#include <cstring>

#include "payload.h"

#define DOSA_COMMS_TRIGGER_SIZE DOSA_COMMS_PAYLOAD_BASE_SIZE + 3

namespace dosa::messages {

enum class TriggerDevice : uint8_t
{
    PIR = 10,
    IR_GRID = 11,
};

class Trigger : public Payload
{
   public:
    Trigger(TriggerDevice device, char const* dvc_id) : Payload("tgr")
    {
        buildBasePayload(payload, DOSA_COMMS_TRIGGER_SIZE);
        std::memcpy(payload + DOSA_COMMS_PAYLOAD_BASE_SIZE, &device, 1);
        std::memcpy(payload + DOSA_COMMS_PAYLOAD_BASE_SIZE + 1, dvc_id, 2);
    }

    [[nodiscard]] char const* getPayload() const override
    {
        return payload;
    }

    [[nodiscard]] uint16_t getPayloadSize() const override
    {
        return DOSA_COMMS_TRIGGER_SIZE;
    }

   private:
    char payload[DOSA_COMMS_TRIGGER_SIZE] = {0};
};

}  // namespace dosa::messages
