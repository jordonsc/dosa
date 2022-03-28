#pragma once

#include <dosa_ranging.h>

#include "sonar_container.h"

namespace dosa {

class SonarApp final : public dosa::RangingApp
{
   public:
    using RangingApp::RangingApp;

   private:
    SonarContainer container;

    bool sensorUpdateReady() override
    {
        return container.getSonar().process();
    }

    uint32_t getSensorDistance() override
    {
        return container.getSonar().getDistance();
    }

    Container& getContainer() override
    {
        return container;
    }

    Container const& getContainer() const override
    {
        return container;
    }
};

}  // namespace dosa
