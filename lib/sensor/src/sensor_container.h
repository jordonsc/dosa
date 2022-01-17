#pragma once

#include <dosa.h>

namespace dosa {

class SensorContainer : public Container
{
   public:
    SensorContainer() : Container(), ir(&serial) {}

    [[nodiscard]] IrGrid& getIrGrid()
    {
        return ir;
    }

   protected:
    IrGrid ir;
};

}  // namespace dosa
