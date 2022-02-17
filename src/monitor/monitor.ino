/**
 * DOSA Display Monitor
 * Croduino_Boards:Inkplate:Inkplate6
 *
 * Uses an Inkplate display to show the status of all DOSA devices.
 */

#include <Arduino.h>
#include <Inkplate.h>

#include "images.h"

Inkplate display(INKPLATE_1BIT);  // Monochrome mode
// Inkplate display(INKPLATE_3BIT);  // Greyscale mode

#define DELAY_MS 5000  // Don't do a full-screen refresh faster than this interval

void setup()
{
    display.begin();
    display.clearDisplay();

    display.drawImage(dosa::dosa_logo, 10, 10, dosa::dosa_logo_width, dosa::dosa_logo_height, BLACK);

    display.setCursor(260, 90);
    display.setTextSize(6);
    display.print("DOSA Monitor");

    display.display();
    delay(5000);
}

void loop()
{
    //
}
