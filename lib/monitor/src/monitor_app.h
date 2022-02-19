#pragma once

#include <dosa_inkplate.h>

#include "const.h"

namespace dosa {

class MonitorApp : public InkplateApp
{
   public:
    explicit MonitorApp(InkplateConfig cfg) : InkplateApp(std::move(cfg), INKPLATE_1BIT) {}

    void init() override
    {
        InkplateApp::init();

        getDisplay().clearDisplay();
        printMain();
        fullRefreshWhenReady();
    }

    void loop() override
    {
        /**
         * Button 1: send trigger
         */
        if (getDisplay().readTouchpad(PAD1)) {
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
        if (getDisplay().readTouchpad(PAD2)) {
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
        if (getDisplay().readTouchpad(PAD3)) {
            refreshDisplay(true);
        }

        delay(100);
    }

   private:
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

        auto& display = getDisplay();
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
};

}  // namespace dosa
