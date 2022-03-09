#pragma once

#include <dosa_inkplate.h>

namespace dosa {

/**
 * Path to config files on the SD card.
 */
namespace config {

char const* wifi = "/config/wifi.txt";

}

/**
 * Path to images on the SD card, and image dimensions.
 */
namespace images {

dimensions const glyph_size = {128, 128};
dimensions const logo_size = {200, 200};
dimensions const panel_size = {780, 140};

char const* logo = "/images/logo_200.png";
char const* sensor_active = "/images/sensor_active_128.png";
char const* sensor_inactive = "/images/sensor_inactive_128.png";
char const* winch_active = "/images/winch_active_128.png";
char const* winch_inactive = "/images/winch_inactive_128.png";
char const* misc_active = "/images/misc_active_128.png";
char const* misc_inactive = "/images/misc_inactive_128.png";
char const* error_active = "/images/warning_active_128.png";
char const* error_inactive = "/images/warning_inactive_128.png";

char const* bat_0 = "/images/bat_0.png";
char const* bat_1 = "/images/bat_1.png";
char const* bat_2 = "/images/bat_2.png";
char const* bat_3 = "/images/bat_3.png";
char const* bat_4 = "/images/bat_4.png";
char const* bat_5 = "/images/bat_5.png";
char const* bat_6 = "/images/bat_6.png";
char const* bat_7 = "/images/bat_7.png";
char const* bat_8 = "/images/bat_8.png";
char const* bat_9 = "/images/bat_9.png";
char const* bat_charge = "/images/bat_x.png";

}  // namespace images

}  // namespace dosa
