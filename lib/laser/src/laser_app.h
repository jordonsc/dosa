#pragma once

#include <dosa_ranging.h>

#include "laser_container.h"

namespace dosa {

class LaserApp final : public dosa::RangingApp
{
   public:
    using dosa::RangingApp::RangingApp;

    void init() override
    {
        RangingApp::init();
        logln("Laser init..");
        container.getLaser().stop();
        container.getLaser().startContinuousMeasurement();
    }

   private:
    LaserContainer container;

    bool sensorUpdateReady() override
    {
        return container.getLaser().process();
    }

    uint32_t getSensorDistance() override
    {
        return container.getLaser().getDistance();
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
