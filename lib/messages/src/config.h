#pragma once

#include <algorithm>
#include <cstring>

#include "const.h"
#include "payload.h"

namespace dosa {
namespace messages {

class Configuration : public Payload
{
   public:
    /**
     * All string encodings should be UTF-8. Do not null-terminate strings unless specified.
     */
    enum class ConfigItem : uint8_t
    {
        /**
         * Byte-string, max 50 bytes.
         */
        PASSWORD = 0,

        /**
         * Byte-string, max 20 bytes.
         */
        DEVICE_NAME = 1,

        /**
         * Byte-string in format SSID\nPASSWORD - no terminator.
         */
        WIFI_AP = 2,

        /**
         * uint8  (1 byte):   min number of pixels changed before triggering
         * float  (4 bytes):  min temperature change before consider pixel changed
         * float  (4 bytes):  overall required temperature change
         */
        PIR_CALIBRATION = 3,

        /**
         * uint16 (2 bytes):  open distance (mm)
         * uint32 (4 bytes):  open-wait time (ms)
         * uint32 (4 bytes):  cool-down time (ms)
         * uint32 (4 bytes):  close ticks
         */
        DOOR_CALIBRATION = 4,

        /**
         * uint16 (2 bytes):  trigger threshold (consecutive reads with split beam)
         * uint16 (2 bytes):  fixed-distance calibration
         * float (4 bytes):   trigger coefficient
         */
        RANGE_CALIBRATION = 5,

        /**
         * uint8 (1 byte):    lock state
         */
        DEVICE_LOCK = 6,

        /**
         * Byte-string, max 500 bytes.
         */
        LISTEN_DEVICES = 7,

        /**
         * uint32 (4 bytes):  relay activation time
         */
        RELAY_CALIBRATION = 8,

        /**
         * uint16 (2 bytes):  server port
         * Byte-string:       server address
         */
        STATS_SERVER = 9,
    };

    /**
     * Create an configuration payload.
     */
    Configuration(void const* cfg, uint16_t cfg_size, char const* dev_name)
        : Payload(DOSA_COMMS_MSG_CONFIG, dev_name),
          payload(DOSA_COMMS_PAYLOAD_BASE_SIZE + cfg_size)
    {
        buildBasePayload(payload);
        payload.set(DOSA_COMMS_PAYLOAD_BASE_SIZE, cfg, cfg_size);
    }

    static Configuration fromPacket(char const* packet, uint32_t size)
    {
        if (size < DOSA_COMMS_PAYLOAD_BASE_SIZE) {
            // cannot log or throw an exception, so create a null Error packet
            return Configuration("", 0, bad_dev_name);
        }

        auto cfg =
            Configuration(packet + DOSA_COMMS_PAYLOAD_BASE_SIZE, size - DOSA_COMMS_PAYLOAD_BASE_SIZE, packet + 7);

        cfg.msg_id = *(uint16_t*)packet;
        cfg.buildBasePayload(cfg.payload);

        return cfg;
    }

    [[nodiscard]] ConfigItem getConfigItem() const
    {
        return static_cast<ConfigItem>(payload.uint8At(DOSA_COMMS_PAYLOAD_BASE_SIZE));
    }

    /**
     * Configuration data size.
     *
     * If `incl_marker` is true then this includes the first ConfigItem byte.
     */
    [[nodiscard]] uint16_t getConfigSize(bool incl_marker = false) const
    {
        if (incl_marker) {
            return payload.getPayloadSize() - DOSA_COMMS_PAYLOAD_BASE_SIZE;
        } else {
            return payload.getPayloadSize() - DOSA_COMMS_PAYLOAD_BASE_SIZE - 1;
        }
    }

    /**
     * If `incl_marker` is true, the full payload including the ConfigItem header is sent.
     * If `incl_marker` is false, the first byte is skipped.
     */
    [[nodiscard]] uint8_t const* getConfigData(bool incl_marker = false) const
    {
        if (incl_marker) {
            return (uint8_t*)(payload.getPayload(DOSA_COMMS_PAYLOAD_BASE_SIZE));
        } else {
            return (uint8_t*)(payload.getPayload(DOSA_COMMS_PAYLOAD_BASE_SIZE + 1));
        }
    }

    [[nodiscard]] char const* getPayload() const override
    {
        return payload.getPayload();
    }

    [[nodiscard]] uint16_t getPayloadSize() const override
    {
        return payload.getPayloadSize();
    }

   protected:
    VariablePayload payload;
};

}  // namespace messages
}  // namespace dosa
