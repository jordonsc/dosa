#pragma once

#include <dosa.h>

#include "laser.h"

namespace dosa {

class LaserContainer : public Container
{
   public:
    LaserContainer() : Container() {}

    [[nodiscard]] Laser& getLaser()
    {
        return laser;
    }

   protected:
    Laser laser;
};

}  // namespace dosa
