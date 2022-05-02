#pragma once

/**
 * Network settings - the number of times we'll send a UDP multicast message without receiving an ack in return before
 * giving up and assuming nobody is listening. Increase this number to deal with poor network transmission.
 *
 * NB: the total number of acks a device will send before giving up is this value + 1 (original request).
 */
#define DOSA_ACK_MAX_RETRIES 4

/**
 * Time in milliseconds to wait for an ack.
 *
 * Max time waiting for an ack is DOSA_ACK_WAIT_TIME * (DOSA_ACK_MAX_RETRIES + 1).
 */
#define DOSA_ACK_WAIT_TIME 100

/**
 * Array size of comms handlers
 */
#define DOSA_COMMS_MAX_HANDLERS 15

namespace dosa {

/**
 * Comms addresses.
 */
namespace comms {

/**
 * Header bytes on a protobuf message.
 */
constexpr char const* DOSA_COMMS_PROTO_HEADER = "DP";

struct Node
{
    // NB: cannot use std::move, no r-value constructor on IPAddress
    Node(IPAddress const& ip, uint16_t port) : ip(ip), port(port) {}

    IPAddress ip;
    uint16_t port;

    bool operator==(Node const& n) const
    {
        return ip == n.ip && port == n.port;
    }

    bool operator!=(Node const& n) const
    {
        return !operator==(n);
    }
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

}  // namespace comms
}  // namespace dosa
