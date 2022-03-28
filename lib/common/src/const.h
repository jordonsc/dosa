#pragma once

#include <Arduino.h>

/**
 * DOSA Application Version.
 */
#ifndef DOSA_VERSION
#define DOSA_VERSION 42
#endif

namespace dosa {

/**
 * Build an Arduino String object from a fixed-size byte array.
 */
[[nodiscard]] String stringFromBytes(void const* bytes, size_t length)
{
    char buffer[length + 1];
    memcpy(buffer, bytes, length);
    buffer[length] = 0;
    return String(buffer);
}

}  // namespace dosa
