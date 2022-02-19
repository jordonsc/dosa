#pragma once

#include <Inkplate.h>
#include <SdFat.h>
#include <WiFi.h>

#include <utility>

#include "config.h"
#include "const.h"
#include "fonts/dejavu_sans_24.h"
#include "fonts/dejavu_sans_48.h"

#define SCREEN_MIN_REFRESH_INT 5000  // Don't do a full-screen refresh faster than this interval
#define SCREEN_MAX_PARTIAL 5         // Number of partial refreshes before forcing a full refresh
#define WIFI_TICKER_DELAY 1000       // Frame-rate for wifi visual indicator

namespace dosa {

class InkplateApp
{
   public:
    InkplateApp(InkplateConfig cfg, uint8_t display_mode) : config(std::move(cfg)), display_mode(display_mode) {}

   protected:
    InkplateConfig config;
    uint8_t display_mode;

    uint32_t last_refresh = 0;   // timestamp to last full refresh
    uint16_t refresh_count = 0;  // counter of partial refreshes

    Inkplate& getDisplay() const
    {
        static Inkplate ink = Inkplate(display_mode);
        return ink;
    }

    WiFiUDP& getUdp() const
    {
        static WiFiUDP udp;
        return udp;
    }

    /**
     * Standard boot-up sequence.
     */
    virtual void init()
    {
        getDisplay().begin();
        getDisplay().clearDisplay();

        getDisplay().setFont(&DejaVu_Sans_48);
        printCentre(config.app_name.c_str(), 360);

        getDisplay().setFont(&DejaVu_Sans_24);
        loadingStatus("Initialising..", 0, false);

        if (getDisplay().sdCardInit() == 0) {
            loadingError("ERROR: SD card init failed!");
        }

        if (!getDisplay().drawPngFromSd(config.logo_filename.c_str(), 300, 120, false, false)) {
            loadingError("ERROR: failed to load graphics");
        }

        getDisplay().display();
        last_refresh = millis();

        loadWifiConfig();
        connectWifi();
        bindMulticast();
    }

    /**
     * Main device loop.
     */
    virtual void loop() = 0;

    /**
     * Performs a partial refresh on the display, unless enough time and iterations have passed in which it will do
     * a full refresh instead.
     */
    void refreshDisplay(bool prefer_full = false)
    {
        if ((millis() - last_refresh > SCREEN_MIN_REFRESH_INT) &&
            ((refresh_count >= SCREEN_MAX_PARTIAL) || prefer_full)) {
            getDisplay().display();
            refresh_count = 0;
            last_refresh = millis();
        } else {
            getDisplay().partialUpdate();
            ++refresh_count;
        }
    }

    /**
     * Waits for SCREEN_MIN_REFRESH_INT then performs a full refresh.
     */
    void fullRefreshWhenReady()
    {
        while (millis() - last_refresh < SCREEN_MIN_REFRESH_INT) {
            delay(50);
        }

        refreshDisplay(true);
    }

    /**
     * Horizontally aligned print.
     */
    void printCentre(char const* txt, uint16_t x, uint16_t y)
    {
        int16_t x1, y1;
        uint16_t w, h;

        getDisplay().getTextBounds(txt, 0, 0, &x1, &y1, &w, &h);
        getDisplay().setCursor(x - (w / 2), y);
        getDisplay().print(txt);
    }

    /**
     * Horizontally aligned print.
     */
    void printCentre(char const* txt, uint16_t y)
    {
        printCentre(txt, dosa::device_size.width / 2, y);
    }

   private:
    /**
     * Loads the wifi AP & password from the text file specified in the config::wifi_filename value.
     */
    void loadWifiConfig()
    {
        FatFile wifi_file;
        if (!wifi_file.open(config.wifi_filename.c_str(), O_RDONLY)) {
            loadingError("ERROR: failed to open wifi.txt");
        }

        int fs = wifi_file.fileSize();
        if (fs > 512) {
            loadingError("Wifi config file exceeds max size (512 bytes)");
        }

        char text[fs + 1];
        wifi_file.read(text, fs);
        text[fs] = 0;
        String cfg(text);

        auto brk = cfg.indexOf("\n");
        if (brk < 0) {
            loadingError("Malformed wifi configuration");
        }

        config.wifi_ap = cfg.substring(0, brk);
        config.wifi_pw = cfg.substring(brk + 1);

        config.wifi_ap.trim();
        config.wifi_pw.trim();
    }

    void bindMulticast()
    {
        getUdp().beginMulticast(comms::mc_address, comms::mc_port);
    }

    void connectWifi()
    {
        loadingStatus("Connecting to " + config.wifi_ap + "..");
        WiFi.begin(config.wifi_ap.c_str(), config.wifi_pw.c_str());

        // Visual connection ticker
        uint8_t wifi_pos = 0;
        while (WiFi.status() != WL_CONNECTED) {
            switch (wifi_pos) {
                default:
                case 0:
                    loadingStatus("Connecting to " + config.wifi_ap + "...", WIFI_TICKER_DELAY);
                    break;
                case 1:
                    loadingStatus("Connecting to " + config.wifi_ap + "....", WIFI_TICKER_DELAY);
                    break;
                case 2:
                    loadingStatus("Connecting to " + config.wifi_ap + ".....", WIFI_TICKER_DELAY);
                    break;
                case 3:
                    loadingStatus("Connecting to " + config.wifi_ap + "..", WIFI_TICKER_DELAY);
                    break;
            }

            if (wifi_pos++ > 3) {
                wifi_pos = 0;
            }
        }

        loadingStatus("Connected");
    }

    /**
     * Display a message during the loading splash screen.
     */
    void loadingStatus(char const* txt, uint16_t wait_time = 0, bool update = true)
    {
        getDisplay().fillRect(0, 380, dosa::device_size.width, 60, WHITE);
        printCentre(txt, 400);

        if (update) {
            // Don't use refreshDisplay() - don't want a full refresh just yet
            getDisplay().partialUpdate();
        }

        if (wait_time > 0) {
            delay(wait_time);
        }
    }

    void loadingStatus(String const& txt, uint16_t wait_time = 0, bool update = true)
    {
        loadingStatus(txt.c_str(), wait_time, update);
    }

    /**
     * Display an error message an indefinitely hold.
     */
    [[noreturn]] void loadingError(char const* error)
    {
        loadingStatus(error, 0, true);
        while (true) {
        }
    }
};

template <class AppT>
class InkAppBuilder final
{
   public:
    InkAppBuilder() = default;

    InkplateConfig& getConfig()
    {
        return config;
    }

    AppT& getApp()
    {
        static AppT app(config);
        return app;
    }

   private:
    InkplateConfig config;
};

}  // namespace dosa
