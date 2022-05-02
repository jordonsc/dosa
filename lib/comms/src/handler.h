#pragma once

#include <Arduino.h>
#include <dosa_messages.h>

#include "const.h"

namespace dosa {
namespace comms {

class Handler
{
   public:
    virtual ~Handler() = default;

    /**
     * Return true if this handler is for legacy messages only. Returning false implies this is a protobuf handler.
     */
    [[nodiscard]] virtual bool isLegacy()
    {
        return true;
    }

    /**
     * The command code this handler responds to.
     */
    [[nodiscard]] virtual String const& getCommandCode() const = 0;

    /**
     * A packet with matching command code has been received, handle appropriately.
     */
    virtual void handlePacket(char const* packet, uint32_t size, Node const&) = 0;
};

}  // namespace comms
}  // namespace dosa