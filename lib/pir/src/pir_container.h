#pragma once

#include <dosa.h>

namespace dosa {

class PirContainer : public Container
{
   public:
    PirContainer() : Container(), ir(&serial) {}

    [[nodiscard]] IrGrid& getIrGrid()
    {
        return ir;
    }

   protected:
    IrGrid ir;
};

}  // namespace dosa
