#pragma once

namespace dosa {

struct dimensions
{
    uint16_t width;
    uint16_t height;
};

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
dimensions const panel_size = {580, 140};
dimensions const device_size = {800, 600};

char const* logo = "/images/logo_200.png";
char const* sensor_active = "/images/sensor_active_128.png";
char const* sensor_inactive = "/images/sensor_inactive_128.png";
char const* winch_active = "/images/winch_active_128.png";
char const* winch_inactive = "/images/winch_inactive_128.png";
char const* misc_active = "/images/misc_active_128.png";
char const* misc_inactive = "/images/misc_inactive_128.png";
char const* error_active = "/images/warning_active_128.png";
char const* error_inactive = "/images/warning_inactive_128.png";

}  // namespace images

}  // namespace dosa