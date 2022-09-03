#pragma once

#ifndef Arduino_h
#include <bits/types.h>
typedef __uint8_t uint8_t;
typedef __uint16_t uint16_t;
typedef __uint32_t uint32_t;
typedef __uint64_t uint64_t;
#endif

// Some generic commands (no additional info outside of the command itself, can use the GenericMessage class)
#define DOSA_COMMS_MSG_BT_MODE "btc"   // request to fallback into Bluetooth config mode
#define DOSA_COMMS_MSG_ONLINE "onl"    // device is online and ready
#define DOSA_COMMS_MSG_BEGIN "bgn"     // device is beginning its primary function
#define DOSA_COMMS_MSG_END "end"       // device has completed its primary function
#define DOSA_COMMS_MSG_PING "pin"      // ping (request for pong)
#define DOSA_COMMS_MSG_OTA "ota"       // request device check for (and install) OTA updates
#define DOSA_COMMS_MSG_DEBUG "dbg"     // request device return log messages containing device state & settings
#define DOSA_COMMS_MSG_FLUSH "fls"     // instruct recipients to flush any cached DOSA data (network reset)
#define DOSA_COMMS_MSG_REQ_STAT "req"  // request the device reply with a full status message

// Command codes with additional information (and their own class)
#define DOSA_COMMS_MSG_LOG "log"       // network-level log
#define DOSA_COMMS_MSG_PONG "pon"      // pong reply from a ping
#define DOSA_COMMS_MSG_TRIGGER "trg"   // sensor/switch has been tripped
#define DOSA_COMMS_MSG_ALT "alt"       // alternative trigger
#define DOSA_COMMS_MSG_CONFIG "cfg"    // update device configuration
#define DOSA_COMMS_MSG_SECURITY "sec"  // security alert
#define DOSA_COMMS_MSG_PLAY "pla"      // request sec-bot run a play
#define DOSA_COMMS_MSG_STATUS "sta"    // full status message

namespace dosa {

enum class LockState : uint8_t
{
    UNLOCKED = 0,  // Device is unlocked and will function normally
    LOCKED = 1,    // Device is locked, will not respond to triggers
    ALERT = 2,     // Device is locked, will send security alerts if triggered
    BREACH = 3,    // Device is locked, will send security breach alerts if triggered
};

enum class SecurityLevel : uint8_t
{
    ALERT = 0,   // Security alert (lock state ALERT tripped)
    BREACH = 1,  // Security breach (lock state BREACH tripped)
    TAMPER = 2,  // Possible device tamper
    PANIC = 3,   // User manually triggered alarm (panic button pressed, etc)
};

namespace messages {
// These packet constants are used in place of an exception, when Payload construction fails
static char const* bad_cmd_code = "NUL";
static char const* bad_dev_name = "BAD_PACKET__________";

/**
 * For use in the StatusMessage payloads.
 */
enum class StatusFormat : uint16_t
{
    STATUS_ONLY = 0,  // Message contains a single 1-byte flag containing the device state
};

}  // namespace messages
}  // namespace dosa
