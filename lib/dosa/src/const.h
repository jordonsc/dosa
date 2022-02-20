#pragma once

/**
 * DOSA Application Version.
 */
#define DOSA_VERSION 22

/**
 * Pin used for CS on FRAM.
 */
#define FRAM_CS_PIN 10

namespace dosa {

/**
 * Build an Arduino String object from a fixed-size byte array.
 */
[[nodiscard]] String stringFromBytes(void const* bytes, size_t length)
{
    char buffer[length + 1];
    memcpy(buffer, bytes, length);
    buffer[length] = 0;
    return String(buffer);
}

/**
 * Bluetooth signatures.
 */
namespace bt {

// DOSA general service
char const* svc_dosa = "d05a0010-e8f2-537e-4f6c-d104768a1000";

// Characteristics
char const* char_version = "d05a0010-e8f2-537e-4f6c-d104768a1001";
char const* char_error_msg = "d05a0010-e8f2-537e-4f6c-d104768a1001";
char const* char_device_name = "d05a0010-e8f2-537e-4f6c-d104768a1002";
char const* char_set_pin = "d05a0010-e8f2-537e-4f6c-d104768a1100";
char const* char_set_wifi = "d05a0010-e8f2-537e-4f6c-d104768a1101";

}  // namespace bt
}  // namespace dosa
