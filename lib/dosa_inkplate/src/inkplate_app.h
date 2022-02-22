#pragma once

#include <Inkplate.h>
#include <SdFat.h>
#include <dosa_common.h>
#include <dosa_comms.h>

#include <utility>

#include "config.h"
#include "const.h"
#include "fonts/dejavu_sans_24.h"
#include "fonts/dejavu_sans_48.h"

#define SCREEN_MIN_REFRESH_INT 5000  // Don't do a full-screen refresh faster than this interval
#define SCREEN_MAX_PARTIAL 5         // Number of partial refreshes before forcing a full refresh

namespace dosa {

class InkplateApp : public virtual Loggable, public NamedApplication, public WifiApplication
{
   public:
    InkplateApp(InkplateConfig cfg, uint8_t display_mode, SerialComms* serial_comms)
        : Loggable(serial_comms),
          NamedApplication("Monitor"),
          WifiApplication(serial_comms),
          config(std::move(cfg)),
          display_mode(display_mode)
    {}

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

    /**
     * Standard boot-up sequence.
     */
    virtual void init()
    {
        // Serial
        serial->setLogLevel(config.log_level);
        if (config.wait_for_serial) {
            // FIXME: this doesn't work on the Inkplate :(
            serial->wait();
        }

        logln("-- " + config.app_name + " --");
#ifdef DOSA_DEBUG
        logln("// Debug Mode //");
#endif
        logln("Init display..");
        getDisplay().begin();

        logln("Init SD card..");
        if (getDisplay().sdCardInit() == 0) {
            loadingError("ERROR: SD card init failed!");
        }

        // We're now ready to paint the splash screen
        printSplash();

        // Wifi initialisation
        loadWifiConfig();

        while (!connectWifi()) {
            // Keep retrying after 30 seconds
            delay(30000);
            printSplash();
        }

        // Bind the DOSA UDP multicast group once connected
        bindMulticast();

        logln("Init complete.");
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

    /**
     * Right aligned print.
     */
    void printRight(char const* txt, uint16_t x, uint16_t y)
    {
        int16_t x1, y1;
        uint16_t w, h;

        getDisplay().getTextBounds(txt, 0, 0, &x1, &y1, &w, &h);
        getDisplay().setCursor(x - w, y);
        getDisplay().print(txt);
    }

    /**
     * Dispatch a generic message on the UDP multicast address.
     */
    void dispatchGenericMessage(char const* cmd_code)
    {
        comms.dispatch(comms::multicastAddr, messages::GenericMessage(cmd_code, getDeviceNameBytes()), false);
    }

    /**
     * Dispatch a specific message on the UDP multicast address.
     */
    void dispatchMessage(messages::Payload const& payload, bool wait_for_ack = false)
    {
        comms.dispatch(comms::multicastAddr, payload, wait_for_ack);
    }

   private:
    /**
     * Startup splash screen
     */
    void printSplash(bool full_refresh = true)
    {
        getDisplay().clearDisplay();

        getDisplay().setFont(&DejaVu_Sans_48);
        printCentre(config.app_name.c_str(), 360);

        if (!getDisplay().drawPngFromSd(config.logo_filename.c_str(), 300, 120, false, false)) {
            loadingError("ERROR: failed to load graphics");
        }

        if (full_refresh) {
            fullRefreshWhenReady();
        }
    }

    /**
     * Loads the wifi AP & password from the text file specified in the config::wifi_filename value.
     */
    void loadWifiConfig()
    {
        logln("Loading wifi config from " + config.wifi_filename);
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

    bool connectWifi()
    {
        logln("Connecting to wifi '" + config.wifi_ap + "'..");
        loadingStatus("Connecting to " + config.wifi_ap + "..");
        wifi.reset();
        wifi.setHostname("monitor.dosa");
        if (wifi.connect(config.wifi_ap.c_str(), config.wifi_pw.c_str())) {
            loadingStatus("Connected");
            return true;
        } else {
            loadingError("Wifi connection failed.", false);
            return false;
        }
    }

    /**
     * Display a message during the loading splash screen.
     */
    void loadingStatus(char const* txt, uint16_t wait_time = 0, bool update = true)
    {
        getDisplay().fillRect(0, 380, dosa::device_size.width, 60, WHITE);
        getDisplay().setFont(&DejaVu_Sans_24);
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
    void loadingError(char const* error, bool no_return = true)
    {
        logln("Load error: " + String(error));

        getDisplay().clearDisplay();
        getDisplay().drawPngFromSd(config.error_filename.c_str(), 335, 235, false, false);
        getDisplay().setFont(&DejaVu_Sans_24);
        printCentre(error, 380);
        getDisplay().display();

        while (no_return) {
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
