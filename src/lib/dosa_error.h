/**
 * Error patterns.
 */

#pragma once

#include "dosa_lights.h"

void successSignal()
{
    auto lights = dosa::Lights::getInstance();
    for (int i = 0; i < 3; ++i) {
        lights.setGreen(true);
        delay(100);
        lights.setGreen(false);
        delay(100);
    }
}

void errorSignal()
{
    auto lights = dosa::Lights::getInstance();
    for (int i = 0; i < 3; ++i) {
        lights.setRed(true);
        delay(100);
        lights.setRed(false);
        delay(100);
    }
}

void waitSignal()
{
    auto lights = dosa::Lights::getInstance();
    for (int i = 0; i < 3; ++i) {
        lights.setBlue(true);
        delay(200);
        lights.setBlue(false);
        delay(400);
    }
}

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
    }
}
