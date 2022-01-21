#pragma once

#ifndef Arduino_h

#include <bits/types.h>

typedef __uint8_t uint8_t;
typedef __uint16_t uint16_t;
typedef __uint32_t uint32_t;
typedef __uint64_t uint64_t;

#endif

#define DOSA_COMMS_CONFIG_MSG_CODE "cfg"  // request to fallback into Bluetooth config mode

namespace dosa::messages {
static char const* bad_cmd_code = "NUL";
static char const* bad_dev_name = "BAD_PACKET__________";
}

#include "ack.h"
#include "generic.h"
#include "trigger.h"
