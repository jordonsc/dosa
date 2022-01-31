#pragma once

#ifndef Arduino_h
#include <bits/types.h>
typedef __uint8_t uint8_t;
typedef __uint16_t uint16_t;
typedef __uint32_t uint32_t;
typedef __uint64_t uint64_t;
#endif

// Some generic commands (no additional info outside of the command itself)
#define DOSA_COMMS_MSG_CONFIG "cfg"  // request to fallback into Bluetooth config mode
#define DOSA_COMMS_MSG_ONLINE "onl"  // device is online and ready
#define DOSA_COMMS_MSG_BEGIN "bgn"  // device is beginning its primary function
#define DOSA_COMMS_MSG_END "end"  // device has completed its primary function
#define DOSA_COMMS_MSG_PING "pin"  // ping (request for pong)

namespace dosa::messages {
static char const* bad_cmd_code = "NUL";
static char const* bad_dev_name = "BAD_PACKET__________";
}  // namespace dosa::messages
