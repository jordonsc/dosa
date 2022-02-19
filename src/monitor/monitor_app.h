#pragma once

#include <Inkplate.h>
#include <SdFat.h>
#include <WiFi.h>

#include "const.h"
#include "fonts/dejavu_sans_24.h"
#include "fonts/dejavu_sans_48.h"

#define SCREEN_MIN_REFRESH_INT 5000  // Don't do a full-screen refresh faster than this interval
#define SCREEN_MAX_PARTIAL 5         // Number of partial refreshes before forcing a full refresh
#define WIFI_TICKER_DELAY 1000       // Frame-rate for wifi visual indicator

namespace dosa {

class MonitorApp
{
   public:
    MonitorApp() : display(INKPLATE_1BIT) {}

    void init()
    {
        display.begin();
        display.clearDisplay();

        display.setFont(&DejaVu_Sans_48);
        printCentre("DOSA Monitor", 360);

        display.setFont(&DejaVu_Sans_24);
        loadStatus("Init SD card..", 0, false);

        display.display();
        last_refresh = millis();

        if (display.sdCardInit() == 0) {
            loadError("ERROR: SD card init failed!");
        }

        if (!display.drawPngFromSd(dosa::images::logo, 300, 120, false, false)) {
            loadError("ERROR: failed to load graphics");
        }

        loadStatus("Read wifi config..", 50);
        if (!wifi_file.open("/config/wifi.txt", O_RDONLY)) {
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

        loadStatus("Connecting to " + wifi_ap + "..");
        WiFi.begin(wifi_ap.c_str(), wifi_pw.c_str());

        auto start = millis();
        while (WiFi.status() != WL_CONNECTED) {
            delay(100);
            if (millis() - start > 3000) {
                // After 3 seconds, start showing the ticker
                break;
            }
        }

        // Visual connection ticker
        uint8_t wifi_pos = 0;
        while (WiFi.status() != WL_CONNECTED) {
            switch (wifi_pos) {
                default:
                case 0:
                    loadStatus("Still connecting /", WIFI_TICKER_DELAY);
                    break;
                case 1:
                    loadStatus("Still connecting -", WIFI_TICKER_DELAY);
                    break;
                case 2:
                    loadStatus("Still connecting \\", WIFI_TICKER_DELAY);
                    break;
                case 3:
                    loadStatus("Still connecting /", WIFI_TICKER_DELAY);
                    break;
            }

            if (wifi_pos++ > 3) {
                wifi_pos = 0;
            }
        }

        loadStatus("Connected to wifi", 500);

        // Done init
        // Wait until SCREEN_MIN_REFRESH_INT between hard refreshes then draw the main screen
        while (millis() - last_refresh < SCREEN_MIN_REFRESH_INT) {
            delay(50);
        }

        display.clearDisplay();
        printMain();
        refreshDisplay(true);
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
            refreshDisplay();
        }

        /**
         * Button 2: refresh devices
         */
        if (display.readTouchpad(PAD2)) {
            device2_state = !device2_state;
            if (device2_state) {
                printPanel(2, dosa::images::winch_active, "Roof Door", "OPEN", true);
            } else {
                printPanel(2, dosa::images::winch_inactive, "Roof Door", "OK");
            }
            refreshDisplay();
        }

        /**
         * Button 3: full-screen refresh
         */
        if (display.readTouchpad(PAD3)) {
            refreshDisplay(true);
        }

        delay(100);
    }

   private:
    Inkplate display;
    FatFile wifi_file;

    uint32_t last_refresh = 0;   // timestamp to last full refresh
    uint16_t refresh_count = 0;  // counter of partial refreshes

    // for testing -
    bool device0_state = false;
    bool device2_state = false;

    /**
     * Print the main display.
     */
    void printMain()
    {
        printPanel(0, dosa::images::sensor_inactive, "Indoor Sensor", "OK");
        printPanel(1, dosa::images::sensor_inactive, "Outdoor Sensor", "OK");
        printPanel(2, dosa::images::winch_inactive, "Roof Door", "OK");
        printPanel(3, dosa::images::misc_inactive, "Monitor", "OK");
    }

    /**
     * Print a device panel, part of the main display.
     */
    void printPanel(uint8_t pos, char const* img, String const& title, String const& status, bool inv = false)
    {
        int x = 10;
        int y = (pos * (dosa::images::panel_size.height + 5)) + 10;  // 5 spacing, 5 top margin

        auto bg = inv ? BLACK : WHITE;
        auto fg = inv ? WHITE : BLACK;

        display.fillRect(x, y, dosa::images::panel_size.width, dosa::images::panel_size.height, bg);
        display.drawRect(x, y, dosa::images::panel_size.width, dosa::images::panel_size.height, fg);

        display.drawPngFromSd(img, x + 5, y + 5, false, inv);

        display.setTextColor(fg, bg);
        display.setTextWrap(false);

        display.setFont(&DejaVu_Sans_48);
        display.setCursor(x + 20 + dosa::images::glyph_size.width, y + 70);
        display.print(title);

        display.setFont(&DejaVu_Sans_24);
        display.setCursor(x + 20 + dosa::images::glyph_size.width, y + 105);
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
        printCentre(txt, dosa::images::device_size.width / 2, y);
    }

    void loadStatus(char const* txt, uint16_t wait_time = 0, bool update = true)
    {
        display.fillRect(0, 380, dosa::images::device_size.width, 60, WHITE);
        printCentre(txt, 400);

        if (update) {
            // Don't use refreshDisplay() - don't want a full refresh just yet
            display.partialUpdate();
        }

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

    /**
     * Performs a partial refresh on the display, unless enough time and iterations have passed in which it will do
     * a full refresh instead.
     */
    void refreshDisplay(bool prefer_full = false)
    {
        if ((millis() - last_refresh > SCREEN_MIN_REFRESH_INT) &&
            ((refresh_count >= SCREEN_MAX_PARTIAL) || prefer_full)) {
            display.display();
            refresh_count = 0;
            last_refresh = millis();
        } else {
            display.partialUpdate();
            ++refresh_count;
        }
    }
};

}  // namespace dosa
