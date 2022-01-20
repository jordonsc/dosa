#pragma once

#include <cstring>
#ifndef Arduino_h
#include <random>
#endif

#define DOSA_COMMS_PAYLOAD_BASE_SIZE 27
#define DOSA_COMMS_MAX_PAYLOAD_SIZE 10240  // 10kib

namespace dosa::messages {

/**
 * Payload for binary transport packet.
 *
 * Format:
 *   Addr   Size   Detail
 *   0      2      Message ID (uint16_t)
 *   2      3      3-byte command code (eg "ack")
 *   5      2      Payload size (uint16_t)
 *   7      20     Device name (null padded)
 *   27+    var    Custom payload data
 */
class Payload
{
   public:
    explicit Payload(char const* cmd_code, char const* dev_name)
    {
#ifdef Arduino_h
        msg_id = random(1, 65535);
#else
        std::random_device rd;
        std::mt19937 mt(rd());
        std::uniform_int_distribution<uint16_t> dist(1, 65535);

        msg_id = dist(mt);
#endif
        std::memcpy(cmd, cmd_code, 3);
        std::memcpy(name, dev_name, 20);
    }

    Payload(uint16_t msgId, char const* cmd_code, char const* dev_name) : msg_id(msgId)
    {
        std::memcpy(cmd, cmd_code, 3);
        std::memcpy(name, dev_name, 20);
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

    /**
     * 3-byte command code.
     */
    [[nodiscard]] virtual char const* getCommandCode() const
    {
        return cmd;
    }

    /**
     * 20-byte null-padded device name.
     */
    [[nodiscard]] virtual char const* getDeviceName() const
    {
        return name;
    }

   protected:
    uint16_t msg_id;
    char cmd[3] = {0};
    char name[20] = {0};

    /**
     * Copy the data for the base of the payload to a char* buffer.
     */
    void buildBasePayload(char* dest, uint16_t size)
    {
        std::memcpy(dest, &msg_id, 2);
        std::memcpy(dest + 2, cmd, 3);
        std::memcpy(dest + 5, &size, 2);
        std::memcpy(dest + 7, name, 20);
    }
};

}  // namespace dosa::messages
