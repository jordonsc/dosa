#pragma once

#include <cstring>
#include <random>

#define DOSA_COMMS_PAYLOAD_BASE_SIZE 7

namespace dosa::messages {

/**
 * Payload for binary transport packet.
 *
 * Format:
 *   Addr   Size   Detail
 *   0      2      Message ID (uint16_t)
 *   2      3      3-byte command code (eg "ack")
 *   5      2      Payload size (uint16_t)
 *   7+     var    Custom payload data
 */
class Payload
{
   public:
    explicit Payload(char const* cmd_code)
    {
        std::random_device rd;
        std::mt19937 mt(rd());
        std::uniform_int_distribution<uint16_t> dist(1, 65535);

        msg_id = dist(mt);
        std::memcpy(cmd, cmd_code, 3);
    }

    Payload(uint16_t msgId, char const* cmd_code) : msg_id(msgId)
    {
        std::memcpy(cmd, cmd_code, 3);
    }

    /**
     * Character string of the entire payload.
     */
    [[nodiscard]] virtual char const* getPayload() const = 0;

    /**
     * 16-bit ID for the message, used for reply messages.
     */
    [[nodiscard]] virtual uint16_t getMessageId() const
    {
        return msg_id;
    }

    /**
     * Full size of the entire payload.
     */
    [[nodiscard]] virtual uint16_t getPayloadSize() const = 0;

   protected:
    uint16_t msg_id;
    char cmd[3] = {0};

    /**
     * Copy the data for the base of the payload to a char* buffer.
     */
    void buildBasePayload(char* dest, uint16_t size)
    {
        std::memcpy(dest, &msg_id, 2);
        std::memcpy(dest + 2, cmd, 3);
        std::memcpy(dest + 5, &size, 2);
    }
};

}  // namespace dosa::messages
