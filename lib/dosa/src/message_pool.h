#pragma once

#include <dosa_comms.h>

#define DOSA_MSG_POOL_SIZE 10

namespace dosa {

struct MessageItem
{
    MessageItem() : sender(IPAddress(), 0), message_id(0) {}
    MessageItem(comms::Node const& sender, uint16_t messageId) : sender(sender), message_id(messageId) {}

    comms::Node sender;
    uint16_t message_id;
};

class MessagePool
{
   public:
    /**
     * Checks if sender+msg combo is registered. Returns true if it's still in cache, else adds and returns false.
     */
    bool validate(comms::Node const& sender, uint16_t msg_id)
    {
        if (isRegistered(sender, msg_id)) {
            return true;
        } else {
            add(sender, msg_id);
            return false;
        }
    }

    bool isRegistered(comms::Node const& sender, uint16_t msg_id)
    {
        auto i = pos;
        do {
            auto const& item = pool[i];
            if (item.sender == sender && item.message_id == msg_id) {
                return true;
            }

            if (++i == DOSA_MSG_POOL_SIZE) {
                i = 0;
            }
        } while (i != pos);

        return false;
    }

    void add(comms::Node const& sender, uint16_t msg_id)
    {
        pool[pos] = {sender, msg_id};

        if (++pos == DOSA_MSG_POOL_SIZE) {
            pos = 0;
        }
    }

   protected:
    uint16_t pos = 0;
    MessageItem pool[DOSA_MSG_POOL_SIZE];
};

}  // namespace dosa
