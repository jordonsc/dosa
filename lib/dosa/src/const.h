#pragma once

/**
 * Pin used for CS on FRAM.
 */
#define FRAM_CS_PIN 10

namespace dosa {

namespace stats {

constexpr char const* online = "dosa.online";
constexpr char const* trigger = "dosa.trigger";
constexpr char const* begin = "dosa.begin";
constexpr char const* end = "dosa.end";
constexpr char const* sequence = "dosa.sequence";
constexpr char const* alt = "dosa.alt";
constexpr char const* ota = "dosa.ota";
constexpr char const* sec_locked = "dosa.security.locked";
constexpr char const* sec_alert = "dosa.security.alert";
constexpr char const* sec_breached = "dosa.security.breached";
constexpr char const* sec_panic = "dosa.security.panic";
constexpr char const* net_ack_retries = "dosa.net.ack.retries";
constexpr char const* net_ack_time = "dosa.net.ack.time";
constexpr char const* net_unacked_triggers = "dosa.net.trigger.unacked";

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
