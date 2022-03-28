#pragma once

/**
 * Pin used for CS on FRAM.
 */
#define FRAM_CS_PIN 10

/**
 * Security alert message for sensors in lock state 'Alert'
 */
#define DOSA_SEC_SENSOR_TRIP "Activity detected"

/**
 * Security alert message for sensors in lock state 'Breach'
 */
#define DOSA_SEC_SENSOR_BREACH "Breach detected"

namespace dosa {

namespace stats {

constexpr char const* online = "dosa.online";
constexpr char const* trigger = "dosa.trigger";
constexpr char const* begin = "dosa.begin";
constexpr char const* end = "dosa.end";
constexpr char const* ota = "dosa.ota";
constexpr char const* sec_locked = "dosa.security.locked";
constexpr char const* sec_alert = "dosa.security.alert";
constexpr char const* sec_breached = "dosa.security.breached";

}  // namespace stats

/**
 * Bluetooth signatures.
 */
namespace bt {

// DOSA general service
constexpr char const* svc_dosa = "d05a0010-e8f2-537e-4f6c-d104768a1000";

// Characteristics
constexpr char const* char_version = "d05a0010-e8f2-537e-4f6c-d104768a1001";
constexpr char const* char_error_msg = "d05a0010-e8f2-537e-4f6c-d104768a1001";
constexpr char const* char_device_name = "d05a0010-e8f2-537e-4f6c-d104768a1002";
constexpr char const* char_set_pin = "d05a0010-e8f2-537e-4f6c-d104768a1100";
constexpr char const* char_set_wifi = "d05a0010-e8f2-537e-4f6c-d104768a1101";

}  // namespace bt
}  // namespace dosa
