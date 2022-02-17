/**
 * DOSA Display Monitor
 * Croduino_Boards:Inkplate:Inkplate6
 *
 * Uses an Inkplate display to show the status of all DOSA devices.
 */

#include <Arduino.h>
#include <Inkplate.h>

#include "images.h"

#define SCREEN_MIN_REFRESH_INT 5000  // Don't do a full-screen refresh faster than this interval

/**
 * Inkplate display in black/white mode.
 */
Inkplate display(INKPLATE_1BIT);

/**
 * Size of a device info panel.
 */
dosa::images::dimensions const panel_size = {580, 140};

auto last_refresh = 0;  // to prevent rapid full-screen refreshing

// for testing -
bool device0_state = false;
bool device2_state = false;


void printDevice(uint8_t pos, uint8_t const* img, String const& title, String const& status, bool inv = false)
{
    int x = 10;
    int y = (pos * (panel_size.height + 5)) + 10;  // 5 spacing, 5 top margin

    auto bg = WHITE;
    auto fg = BLACK;

    if (inv) {
        bg = BLACK;
        fg = WHITE;
    }

    display.fillRect(x, y, panel_size.width, panel_size.height, bg);
    display.drawRect(x, y, panel_size.width, panel_size.height, fg);
    display.drawImage(img, x + 5, y + 5, dosa::images::glyph.width, dosa::images::glyph.height, fg, bg);

    display.setTextColor(fg, bg);
    display.setTextWrap(false);

    display.setCursor(x + 20 + dosa::images::glyph.width, y + 35);
    display.setTextSize(5);
    display.print(title);

    display.setCursor(x + 20 + dosa::images::glyph.width, y + 85);
    display.setTextSize(3);
    display.print(status);
}

void setup()
{
    display.begin();
    display.clearDisplay();

    // Splash screen
    display.drawImage(dosa::images::dosa_logo, 300, 120, dosa::images::logo.width, dosa::images::logo.height, BLACK);
    display.setCursor(220, 350);
    display.setTextSize(6);
    display.print("DOSA Monitor");

    display.display();
    last_refresh = millis();
    delay(3000);

    display.fillRect(0, 0, 800, 600, WHITE);

    printDevice(0, dosa::images::sensor_inactive, "Indoor Sensor", "OK");
    printDevice(1, dosa::images::sensor_inactive, "Outdoor Sensor", "OK");
    printDevice(2, dosa::images::door_inactive, "Garage Door", "OK");
    printDevice(3, dosa::images::door_inactive, "Roof Door", "OK");

    display.partialUpdate();
}

void loop()
{
    /**
     * Button 1: send trigger
     */
    if (display.readTouchpad(PAD1)) {
        device0_state = !device0_state;
        if (device0_state) {
            printDevice(0, dosa::images::sensor_active, "Indoor Sensor", "MOTION DETECTED", true);
        } else {
            printDevice(0, dosa::images::sensor_inactive, "Indoor Sensor", "OK");
        }
        display.partialUpdate();
    }

    /**
     * Button 2: refresh devices
     */
    if (display.readTouchpad(PAD2)) {
        device2_state = !device2_state;
        if (device2_state) {
            printDevice(2, dosa::images::door_active, "Garage Door", "OPEN", true);
        } else {
            printDevice(2, dosa::images::door_inactive, "Garage Door", "OK");
        }
        display.partialUpdate();
    }

    /**
     * Button 3: full-screen refresh
     */
    if (display.readTouchpad(PAD3)) {
        if (millis() - last_refresh > SCREEN_MIN_REFRESH_INT) {
            display.display();
            last_refresh = millis();
        }
    }

    delay(100);
}
