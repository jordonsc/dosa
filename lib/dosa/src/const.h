/**
 * DOSA common constants.
 */

#pragma once

// Quote hack for -D strings
#define DOSA_QUOTE_Q(x) #x
#define DOSA_QUOTE(x) DOSA_QUOTE_Q(x)

/**
 * DOSA Application Version.
 */
#define DOSA_VERSION 22

/**
 * Pin used for CS on FRAM.
 */
#define FRAM_CS_PIN 10

/**
 * Network settings - the number of times we'll send a UDP multicast message without receiving an ack in return before
 * giving up and assuming nobody is listening. Increase this number to deal with poor network transmission.
 */
#define DOSA_ACK_MAX_RETRIES 3

/**
 * Time in milliseconds to wait for an ack.
 */
#define DOSA_ACK_WAIT_TIME 750

/**
 * Array size of comms handlers
 */
#define DOSA_COMMS_MAX_HANDLERS 10

/**
 * Bluetooth signatures.
 */
namespace dosa::bt {

// DOSA general service
char const* svc_dosa = "d05a0010-e8f2-537e-4f6c-d104768a1000";

// Characteristics
char const* char_version = "d05a0010-e8f2-537e-4f6c-d104768a1001";
char const* char_error_msg = "d05a0010-e8f2-537e-4f6c-d104768a1001";
char const* char_device_name = "d05a0010-e8f2-537e-4f6c-d104768a1002";
char const* char_set_pin = "d05a0010-e8f2-537e-4f6c-d104768a1100";
char const* char_set_wifi = "d05a0010-e8f2-537e-4f6c-d104768a1101";

}  // namespace dosa::bt

namespace dosa {
[[nodiscard]] String stringFromBytes(void const* bytes, size_t length)
{
    char buffer[length + 1];
    memcpy(buffer, bytes, length);
    buffer[length] = 0;
    return String(buffer);
}
}  // namespace dosa

/**
 * Comms addresses.
 */
namespace dosa::comms {

struct Node
{
    Node(IPAddress ip, uint16_t port) : ip(std::move(ip)), port(port) {}

    IPAddress ip;
    uint16_t port;
};

Node const multicastAddr(IPAddress(239, 1, 1, 69), 6901);
uint16_t const udpPort = 6902;

[[nodiscard]] String ipToString(IPAddress const& ip)
{
    return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
}

[[nodiscard]] String nodeToString(comms::Node const& node)
{
    return ipToString(node.ip) + ":" + String(node.port);
}

}  // namespace dosa::comms
