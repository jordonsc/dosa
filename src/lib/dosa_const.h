/**
 * DOSA common constants.
 */

#pragma once

#define DOSA_MAX_PERIPHERALS 5  // Max number of peripherals that controllers will connect to
#define DOSA_SCAN_FREQ 5000     // How often we scan for new peripherals
#define DOSA_POLL_FREQ 1000     // How often we poll the peripherals
#define DOSA_BT_DATA_MIN 400    // Bluetooth min comms speed (milliseconds = value * 1.25) - min 6 (7.5 ms)
#define DOSA_BT_DATA_MAX 3200   // Bluetooth max comms speed (milliseconds = value * 1.25) - max 3200 (4 seconds)

namespace dosa {

char const* sensor_svc_id = "19b10000-e8f2-537e-4f6c-d104768a1214";
char const* sensor_char_id = "19b10001-e8f2-537e-4f6c-d104768a1214";

}  // namespace dosa
