/**
 * Error patterns.
 */

#pragma once

#include "dosa_lights.h"

/**
 * Loops indefinitely in an error pattern suggesting an 'init' error.
 */
void errorPatternInit()
{
    auto lights = dosa::Lights::getInstance();
    lights.setLights(true, false, false, false);

    while (true) {
        lights.setRed(true);
        delay(500);
        lights.setRed(true);
        delay(500);
    };
}
