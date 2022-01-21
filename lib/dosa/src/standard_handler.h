#pragma once

#include <utility>

#include "handler.h"

namespace dosa::comms {

/**
 * Templated handler for common scenarios.
 *
 * PayloadClass class must implement `::fromPacket(char const*)`
 */
template <class PayloadClass>
class StandardHandler : public dosa::comms::Handler
{
   public:
    typedef void (*handlerCallback)(PayloadClass const&, Node const&, void*);

    explicit StandardHandler(String cmd, handlerCallback callback = nullptr, void* context = nullptr)
        : cmd_code(std::move(cmd)),
          ctx(context),
          cb(callback)
    {}

    [[nodiscard]] String const& getCommandCode() const override
    {
        return cmd_code;
    }

    void handlePacket(const char* packet, uint32_t size, Node const& sender) override
    {
        if (cb == nullptr) {
            return;
        }

        cb(PayloadClass::fromPacket(packet, size), sender, ctx);
    }

    void setCallback(handlerCallback callback, void* context = nullptr)
    {
        cb = callback;
        ctx = context;
    }

   protected:
    String cmd_code;
    handlerCallback cb;
    void* ctx;
};

}  // namespace dosa::comms
