/**
 * DOSA Display Monitor
 * Croduino_Boards:Inkplate:Inkplate6
 *
 * Uses an Inkplate display to show the status of all DOSA devices.
 */

#include <Arduino.h>
#include <Inkplate.h>

#include "SdFat.h"
#include "fonts/dejavu_sans_24.h"
#include "fonts/dejavu_sans_48.h"
#include "images.h"
SdFile wifi_file;

#define SCREEN_MIN_REFRESH_INT 5000  // Don't do a full-screen refresh faster than this interval

/**
 * Inkplate display in black/white mode.
 *
 * Options are INKPLATE_1BIT or INKPLATE_3BIT. Note that 3BIT doesn't support partial updates.
 */
Inkplate display(INKPLATE_1BIT);

/**
 * Size of a device info panel.
 */
dosa::images::dimensions const panel_size = {580, 140};

/**
 * Size of the physical device.
 *
 * This is the Inkplate resolution.
 */
dosa::images::dimensions const device_size = {800, 600};

auto last_refresh = 0;  // to prevent rapid full-screen refreshing

// for testing -
bool device0_state = false;
bool device2_state = false;

void printPanel(uint8_t pos, uint8_t const* img, String const& title, String const& status, bool inv = false)
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

void printCentre(char const* txt, uint16_t x, uint16_t y)
{
    int16_t x1, y1;
    uint16_t w, h;

    display.getTextBounds(txt, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(x - (w / 2), y);
    display.print(txt);
}

void printCentre(char const* txt, uint16_t y)
{
    printCentre(txt, device_size.width / 2, y);
}

void loadStatus(char const* txt, uint16_t wait_time = 0)
{
    display.fillRect(0, 380, device_size.width, 60, WHITE);
    printCentre(txt, 400);
    display.partialUpdate();

    if (wait_time > 0) {
        delay(wait_time);
    }
}

void loadStatus(String const& txt, uint16_t wait_time = 0)
{
    loadStatus(txt.c_str(), wait_time);
}

[[noreturn]] void loadError(char const* error)
{
    loadStatus(error);
    while (true) {
    }
}

void setup()
{
    display.begin();
    display.clearDisplay();

    // Splash screen
    display.drawImage(dosa::images::dosa_logo, 300, 120, dosa::images::logo.width, dosa::images::logo.height, BLACK);

    display.setFont(&DejaVu_Sans_48);
    printCentre("DOSA Monitor", 360);

    display.display();
    last_refresh = millis();
    delay(500);

    display.setFont(&DejaVu_Sans_24);
    loadStatus("Checking SD card..", 250);

    if (display.sdCardInit() == 1) {
        loadStatus("SD card init success", 500);
    } else {
        loadError("ERROR: SD card init failed!");
    }

    if (!wifi_file.open("/wifi.txt", O_RDONLY)) {
        loadError("ERROR: failed to open wifi.txt");
    }

    int fs = wifi_file.fileSize();
    if (fs > 512) {
        loadError("Wifi config file exceeds max size (512 bytes)");
    }

    char text[fs + 1];
    wifi_file.read(text, fs);
    text[fs] = 0;
    String cfg(text);
    auto brk = cfg.indexOf("\n");
    if (brk < 0) {
        loadError("Malformed wifi configuration");
    }
    auto wifi_ap = cfg.substring(0, brk);
    auto wifi_pw = cfg.substring(brk + 1);

    wifi_ap.trim();
    wifi_pw.trim();

    loadStatus("Wifi AP: '" + wifi_ap + "', pw: '" + wifi_pw + "'", 3000);

    return;

    delay(3000);
    display.fillRect(0, 0, 800, 600, WHITE);

    printPanel(0, dosa::images::sensor_inactive, "Indoor Sensor", "OK");
    printPanel(1, dosa::images::sensor_inactive, "Outdoor Sensor", "OK");
    printPanel(2, dosa::images::door_inactive, "Garage Door", "OK");
    printPanel(3, dosa::images::door_inactive, "Roof Door", "OK");

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
            printPanel(0, dosa::images::sensor_active, "Indoor Sensor", "MOTION DETECTED", true);
        } else {
            printPanel(0, dosa::images::sensor_inactive, "Indoor Sensor", "OK");
        }
        display.partialUpdate();
    }

    /**
     * Button 2: refresh devices
     */
    if (display.readTouchpad(PAD2)) {
        device2_state = !device2_state;
        if (device2_state) {
            printPanel(2, dosa::images::door_active, "Garage Door", "OPEN", true);
        } else {
            printPanel(2, dosa::images::door_inactive, "Garage Door", "OK");
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
