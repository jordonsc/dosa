/**
 * DOSA Display Monitor
 * Croduino_Boards:Inkplate:Inkplate6
 *
 * Uses an Inkplate display to show the status of all DOSA devices.
 */

#include <Inkplate.h>
#include "monitor_app.h"

auto app = dosa::MonitorApp();

/**
 * Arduino setup
 */
void setup()
{
    app.init();
}

/**
 * Arduino main loop
 */
void loop()
{
    app.loop();
}
