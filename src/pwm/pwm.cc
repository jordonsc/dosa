/**
 * DOSA PWM Relay Controller
 * arduino:megaavr:nona4809
 *
 * Reads a value via Serial and outputs a hardware PWM signal.
 */

#include <dosa_pwm.h>

auto app = dosa::PwmApp();

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
