#pragma once

#include <Arduino.h>
#include <messages.h>

#include "const.h"

namespace dosa {
namespace comms {

class Handler
{
   public:
    virtual ~Handler() = default;

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