#pragma once

#include <Arduino.h>
#include <messages.h>

namespace dosa::comms {

class Handler
{
   public:
    virtual ~Handler() = default;

    /**
     * The command code this handler responds to.
     */
    virtual String getCommandCode() = 0;

    /**
     * A packet with matching command code has been received, handle appropriately.
     */
    virtual void handlePacket(char const *packet, uint32_t size) = 0;
};

}  // namespace dosa::comms
