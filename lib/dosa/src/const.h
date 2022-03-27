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
