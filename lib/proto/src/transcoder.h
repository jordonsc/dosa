#pragma once

#include "pb_common.h"
#include "pb_decode.h"

namespace dosa {

class ProtoTranscoder
{
   public:
    bool decode(void* dest, pb_msgdesc_t const* fields, uint8_t* buffer, size_t size) const
    {
        pb_istream_t stream = pb_istream_from_buffer(buffer, size);
        return pb_decode(&stream, fields, dest);
    }
};

}  // namespace dosa