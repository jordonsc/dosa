#pragma once

#include <messages.h>

#include "const.h"
#include "standard_handler.h"
#include "wifi.h"

namespace dosa {

class Comms : public Loggable
{
   public:
    explicit Comms(Wifi& wifi, SerialComms* s = nullptr) : Loggable(s), wifi(wifi)
    {
        // Create an internal handler for Ack messages
        newHandler<comms::StandardHandler<messages::Ack>>(DOSA_COMMS_ACK_MSG_CODE, &ackMessageForwarder, this);
    }

    virtual ~Comms()
    {
        for (auto& handler : handlers) {
            delete handler;
        }
    }

    /**
     * Listen on wifi UDP port.
     */
    bool bind(uint16_t port)
    {
        return wifi.getUdp().begin(port);
    }

    /**
     * Listen for UDP multicast message on wifi.
     */
    bool bindMulticast(IPAddress const& ip, uint16_t port)
    {
        return wifi.getUdp().beginMulticast(ip, port);
    }

    bool bindMulticast(comms::Node const& node)
    {
        return wifi.getUdp().beginMulticast(node.ip, node.port);
    }

    /**
     * Registers and takes memory ownership of given message handler.
     *
     * NB: if this returns false, the handler has NOT been registered and is NOT under memory management! (fatal error)
     * Consider `newHandler(...)` instead of this.
     */
    bool registerHandler(comms::Handler* h)
    {
        for (auto& handler : handlers) {
            if (handler == nullptr) {
                handler = h;
                return true;
            }
        }

        logln("CRITICAL: no free space for new comms handler!", dosa::LogLevel::CRITICAL);
        return false;
    }

    /**
     * Ideal way to register a new handler, will construct on the spot and fully manage memory.
     *
     * Returns a pointer to the new object if successful, a nullptr if there isn't space.
     */
    template <class T, class... Args>
    comms::Handler* newHandler(Args... args)
    {
        T* handler = new T(args...);

        if (registerHandler(handler)) {
            return handler;
        } else {
            delete handler;
            return nullptr;
        }
    }

    /**
     * Dispatch a message.
     *
     * If `wait_for_ack` is true, this call will block until a return `ack` packet is received and re-send the message
     * every DOSA_ACK_WAIT_TIME ms until DOSA_ACK_MAX_RETRIES is exhausted.
     *
     * Returns true if successfully sent [and ack'd], false if not ack'd or not sent successfully.
     */
    bool dispatch(comms::Node const& recipient, messages::Payload const& payload, bool wait_for_ack = false)
    {
        return dispatch(recipient.ip, recipient.port, payload, wait_for_ack);
    }

    bool dispatch(IPAddress const& ip, unsigned long port, messages::Payload const& payload, bool wait_for_ack = false)
    {
        return dispatchMsg(ip, port, payload, wait_for_ack);
    }

    /**
     * Process inbound packets.
     *
     * Fires events if a packet is received.
     */
    bool processInbound()
    {
        if (!wifi.isConnected()) {
            return false;
        }

        auto& udp = wifi.getUdp();

        if (!udp.parsePacket()) {
            return false;
        }

        auto data_size = uint32_t(udp.available());
        if (data_size < DOSA_COMMS_PAYLOAD_BASE_SIZE) {
            logln("Inbound packet under min size, flushing", dosa::LogLevel::WARNING);
            udp.flush();
            return false;
        } else if (data_size > DOSA_COMMS_MAX_PAYLOAD_SIZE) {
            logln("Inbound packet exceeds max capacity, flushing", dosa::LogLevel::WARNING);
            udp.flush();
            return false;
        }

        char buffer[data_size];
        udp.read(buffer, data_size);

        // Convert command code bytes into Arduino string format
        char cmd_raw[4] = {0};
        memcpy(cmd_raw, buffer + 2, 3);
        String cmd_code(cmd_raw);

        handlePacket(cmd_code, buffer, data_size, comms::Node(udp.remoteIP(), udp.remotePort()));
        return true;
    }

    /**
     * Get the command code from a payload as a String.
     */
    static String getCommandCode(messages::Payload const& payload)
    {
        char buffer[4] = {0};
        memcpy(buffer, payload.getCommandCode(), 3);
        return String(buffer);
    }

    /**
     * Get the device name from a payload as a String.
     */
    static String getDeviceName(messages::Payload const& payload)
    {
        char buffer[21] = {0};
        memcpy(buffer, payload.getDeviceName(), 20);
        return String(buffer);
    }

   protected:
    Wifi& wifi;
    comms::Handler* handlers[DOSA_COMMS_MAX_HANDLERS] = {nullptr};
    uint16_t ack_msg_id = 0;

    /**
     * Search registered message handlers and tell them to handle any matches.
     */
    void handlePacket(String const& cmd, char const* packet, uint32_t size, comms::Node const& sender)
    {
        for (auto& handler : handlers) {
            if (handler != nullptr && handler->getCommandCode() == cmd) {
                handler->handlePacket(packet, size, sender);
            }
        }
    }

    /**
     * Recursive dispatch, retrying until an ack is received or attempts expire.
     */
    bool dispatchMsg(
        IPAddress const& ip,
        unsigned long port,
        messages::Payload const& payload,
        bool wait_for_ack,
        size_t retries = 0)
    {
        if (!wifi.isConnected()) {
            return false;
        }

        auto& udp = wifi.getUdp();
        ack_msg_id = 0;

        if (udp.beginPacket(ip, port) != 1) {
            logln("ERROR: UDP begin failed", dosa::LogLevel::ERROR);
            return false;
        }

#ifdef ARDUINO_ARCH_SAMD
        udp.write(payload.getPayload(), payload.getPayloadSize());
#endif
#ifdef ARDUINO_ESP32_DEV
        udp.write((uint8_t const*)(payload.getPayload()), payload.getPayloadSize());
#endif

        if (udp.endPacket() != 1) {
            logln("ERROR: UDP end failed", dosa::LogLevel::ERROR);
            return false;
        }

        logln(
            "SEND: " + getCommandCode(payload) + " to " + comms::ipToString(ip) + ":" + String(port),
            LogLevel::TRACE);

        // (semi) block until we receive an ack (non-ack inbound messages will interrupt)
        if (wait_for_ack) {
            auto start_time = millis();
            do {
                // Process inbound messages, an internal ack handler will set `ack_msg_id` as they come in
                processInbound();

                if (ack_msg_id == payload.getMessageId()) {
                    return true;
                }

            } while (millis() - start_time < DOSA_ACK_WAIT_TIME);

            // Check if we're giving up
            if (retries == DOSA_ACK_MAX_RETRIES) {
                logln("Failed to receive ack message for " + getCommandCode(payload), LogLevel::WARNING);
                return false;
            }

            // Retry via recursion
            return dispatchMsg(ip, port, payload, true, retries + 1);
        }
    }

    /**
     * An ack has been received.
     *
     * Set a flag, monitored by a loop in the dispatch function.
     */
    void onAck(dosa::messages::Ack const& ack, comms::Node const& sender)
    {
        logln(
            "Received ack from '" + Comms::getDeviceName(ack) + "' (" + comms::nodeToString(sender) + ")",
            LogLevel::TRACE);

        ack_msg_id = ack.getAckMsgId();
    }

    /**
     * Context forwarder for ack messages.
     */
    static void ackMessageForwarder(dosa::messages::Ack const& ack, comms::Node const& sender, void* context)
    {
        static_cast<Comms*>(context)->onAck(ack, sender);
    }
};

}  // namespace dosa
