#pragma once

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

namespace dosa {

/**
 * Comms addresses.
 */
namespace comms {

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
