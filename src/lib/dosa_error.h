/**
 * Error patterns.
 *
 * Use these to signal an error pattern using device lights.
 */

#pragma once

#include "dosa_lights.h"

void successSignal()
{
    auto& lights = dosa::Lights::getInstance();
    for (int i = 0; i < 3; ++i) {
        lights.setGreen(true);
        delay(100);
        lights.setGreen(false);
        delay(100);
    }
}

void warnSignal()
{
    auto& lights = dosa::Lights::getInstance();
    for (int i = 0; i < 3; ++i) {
        lights.setGreen(true);
        lights.setBlue(true);
        delay(100);
        lights.setGreen(false);
        lights.setBlue(true);
        delay(100);
    }
}

void errorSignal()
{
    auto& lights = dosa::Lights::getInstance();
    for (int i = 0; i < 3; ++i) {
        lights.setRed(true);
        delay(100);
        lights.setRed(false);
        delay(100);
    }
}

void waitSignal()
{
    auto& lights = dosa::Lights::getInstance();
    lights.set(false, true, false, true);
}

/**
 * Loops indefinitely in an error pattern suggesting an 'init' error.
 */
void errorHoldingPattern()
{
    auto& lights = dosa::Lights::getInstance();

    while (true) {
        lights.set(true, true, false, false);
        delay(500);
        lights.off();
        delay(500);
    }
}
