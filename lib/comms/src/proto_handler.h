#pragma once

#include <dosa_proto.h>

#include <utility>

#include "handler.h"

namespace dosa {
namespace comms {

/**
 * Handler for protobuf messages.
 */
template <class ProtoClass>
class ProtoHandler : public dosa::comms::Handler
{
   public:
    typedef void (*handlerCallback)(ProtoClass const&, Node const&, void*);

    explicit ProtoHandler(
        String cmd,
        pb_msgdesc_t const* fields,
        handlerCallback callback = nullptr,
        void* context = nullptr)
        : cmd_code(std::move(cmd)),
          ctx(context),
          cb(callback),
          fields((pb_msgdesc_t*)fields)
    {}

    bool isLegacy() override
    {
        return false;
    }

    [[nodiscard]] String const& getCommandCode() const override
    {
        return cmd_code;
    }

    void handlePacket(char const* packet, uint32_t size, Node const& sender) override
    {
        if (cb == nullptr) {
            return;
        }

        ProtoClass msg = {};
        transcoder.decode(&msg, fields, packet, size);
        cb(msg, sender, ctx);
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
    ProtoTranscoder transcoder;
    pb_msgdesc_t* fields;
};

}  // namespace comms
}  // namespace dosa
