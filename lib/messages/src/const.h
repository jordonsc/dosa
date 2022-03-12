#pragma once

#ifndef Arduino_h
#include <bits/types.h>
typedef __uint8_t uint8_t;
typedef __uint16_t uint16_t;
typedef __uint32_t uint32_t;
typedef __uint64_t uint64_t;
#endif

// Some generic commands (no additional info outside of the command itself, can use the GenericMessage class)
#define DOSA_COMMS_MSG_BT_MODE "btc"  // request to fallback into Bluetooth config mode
#define DOSA_COMMS_MSG_ONLINE "onl"  // device is online and ready
#define DOSA_COMMS_MSG_BEGIN "bgn"  // device is beginning its primary function
#define DOSA_COMMS_MSG_END "end"  // device has completed its primary function
#define DOSA_COMMS_MSG_PING "pin"  // ping (request for pong)
#define DOSA_COMMS_MSG_OTA "ota"  // request device check for (and install) OTA updates
#define DOSA_COMMS_MSG_DEBUG "dbg"  // request device return log messages containing device state & settings

// Command codes with additional information (and their own class)
#define DOSA_COMMS_MSG_LOG "log"  // network-level log
#define DOSA_COMMS_MSG_PONG "pon"  // pong reply from a ping
#define DOSA_COMMS_MSG_TRIGGER "trg"  // sensor/switch has been tripped
#define DOSA_COMMS_MSG_CONFIG "cfg"  // update device configuration

namespace dosa {
namespace messages {
// These packet constants are used in place of an exception, when Payload construction fails
static char const* bad_cmd_code = "NUL";
static char const* bad_dev_name = "BAD_PACKET__________";
}  // namespace messages
}  // namespace dosa
