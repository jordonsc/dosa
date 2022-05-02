#pragma once

#include <algorithm>
#include <cstring>
#ifndef Arduino_h
#include <random>
#endif

#include "const.h"

#define DOSA_COMMS_PAYLOAD_BASE_SIZE 27
#define DOSA_COMMS_MAX_PAYLOAD_SIZE 10240  // 10kib
#define DOSA_COMMS_PB_MIN_SIZE 9

namespace dosa {
namespace messages {

/**
 * Contains a payload of variable size.
 *
 * Copy & movable.
 */
class VariablePayload final
{
   public:
    VariablePayload(uint16_t payload_size, void const* src) : payload_size(payload_size)
    {
        payload = new char[payload_size];
        memcpy(payload, src, payload_size);
    }

    explicit VariablePayload(uint16_t payload_size) : payload_size(payload_size)
    {
        payload = new char[payload_size];
    }

    VariablePayload(VariablePayload const& p) : payload_size(p.payload_size)
    {
        payload_size = p.payload_size;
        payload = new char[payload_size];
        memcpy(payload, p.payload, payload_size);
    }

    VariablePayload& operator=(VariablePayload p)
    {
        swap(*this, p);
        return *this;
    }

    friend void swap(VariablePayload& first, VariablePayload& second)
    {
        using std::swap;

        swap(first.payload, second.payload);
        swap(first.payload_size, second.payload_size);
    }

    virtual ~VariablePayload()
    {
        delete payload;
    }

    [[nodiscard]] char* getPayload(uint16_t offset = 0) const
    {
        return payload + offset;
    }

    [[nodiscard]] uint16_t getPayloadSize() const
    {
        return payload_size;
    }

    [[nodiscard]] uint8_t uint8At(uint16_t pos) const
    {
        uint8_t value;
        memcpy(&value, payload + pos, 1);
        return value;
    }

    [[nodiscard]] uint16_t uint16At(uint16_t pos) const
    {
        uint16_t value;
        memcpy(&value, payload + pos, 2);
        return value;
    }

    [[nodiscard]] uint32_t uint32At(uint16_t pos) const
    {
        uint32_t value;
        memcpy(&value, payload + pos, 4);
        return value;
    }

    void set(uint16_t pos, uint8_t value)
    {
        memcpy(payload + pos, &value, 1);
    }

    void set(uint16_t pos, uint16_t value)
    {
        memcpy(payload + pos, &value, 2);
    }

    void set(uint16_t pos, uint32_t value)
    {
        memcpy(payload + pos, &value, 4);
    }

    void set(uint16_t pos, void const* value, uint16_t size)
    {
        memcpy(payload + pos, value, size);
    }

   private:
    char* payload;
    uint16_t payload_size;
};

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
    Payload(char const* cmd_code, char const* dev_name)
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

    void copyFrom(Payload const& p)
    {
        msg_id = p.msg_id;
        std::copy(p.cmd, p.cmd + 3, cmd);
        std::copy(p.name, p.name + 20, name);
    }

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

    void buildBasePayload(VariablePayload& p)
    {
        buildBasePayload(p.getPayload(), p.getPayloadSize());
    }
};

}  // namespace messages
}  // namespace dosa
