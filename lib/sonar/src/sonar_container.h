#pragma once

#include <dosa.h>

namespace dosa {

class SonarContainer : public Container
{
   public:
    SonarContainer() : Container() {}

    [[nodiscard]] Sonar& getSonar()
    {
        return sonar;
    }

   protected:
    Sonar sonar;
};

}  // namespace dosa
