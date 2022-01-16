/**
 * DOSA common constants.
 */

#pragma once

// Quote hack for -D strings
#define DOSA_QUOTE_Q(x) #x
#define DOSA_QUOTE(x) DOSA_QUOTE_Q(x)

// DOSA platform version
#define DOSA_VERSION 5

// General configuration
#define DOSA_MAX_PERIPHERALS 5  // Max number of peripherals that centrals will connect to
#define DOSA_SCAN_FREQ 5000     // How often we scan for new peripherals
#define DOSA_POLL_FREQ 1000     // How often we poll the peripherals for updates

// Unused
#define DOSA_BT_DATA_MIN 400   // Bluetooth min comms speed (milliseconds = value * 1.25) - min 6 (7.5 ms)
#define DOSA_BT_DATA_MAX 3200  // Bluetooth max comms speed (milliseconds = value * 1.25) - max 3200 (4 seconds)

/**
 * Bluetooth signatures.
 */
namespace dosa::bt {

// DOSA general service
char const* svc_dosa = "d05a0010-e8f2-537e-4f6c-d104768a1000";

// Characteristics
char const* char_version = "d05a0010-e8f2-537e-4f6c-d104768a1001";
char const* char_error_msg = "d05a0010-e8f2-537e-4f6c-d104768a1001";
char const* char_set_pin = "d05a0010-e8f2-537e-4f6c-d104768a1100";
char const* char_set_wifi = "d05a0010-e8f2-537e-4f6c-d104768a1101";

}  // namespace dosa::bt

/**
 * Wifi consts.
 */
namespace dosa::wifi {

IPAddress const sensorBroadcastIp = IPAddress(239, 1, 1, 69);
uint16_t const sensorBroadcastPort = 6901;

}  // namespace dosa::wifi
