#pragma once

#include <Inkplate.h>

namespace dosa {

struct dimensions
{
    uint16_t width;
    uint16_t height;
};

dimensions const device_size = {800, 600};

namespace comms {

auto const mc_address = IPAddress(239, 1, 1, 69);
uint16_t const mc_port = 6901;

}  // namespace comms

}  // namespace dosa
