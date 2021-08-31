/**
 * DOSA common constants.
 */

#pragma once

#define DOSA_MAX_PERIPHERALS 5  // Max number of peripherals that centrals will connect to
#define DOSA_SCAN_FREQ 5000     // How often we scan for new peripherals
#define DOSA_POLL_FREQ 1000     // How often we poll the peripherals for updates
#define DOSA_BT_DATA_MIN 400    // Bluetooth min comms speed (milliseconds = value * 1.25) - min 6 (7.5 ms)
#define DOSA_BT_DATA_MAX 3200   // Bluetooth max comms speed (milliseconds = value * 1.25) - max 3200 (4 seconds)

namespace dosa::bt {

// Sensor service
char const* svc_sensor = "d05a0010-e8f2-537e-4f6c-d104768a1000";

// Motion sensor characteristic
char const* char_pir = "d05a0010-e8f2-537e-4f6c-d104768a1010";

}  // namespace dosa::bt
