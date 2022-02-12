/**
 * Sparkfun GridEYE.
 *
 * 8x8 infrared grid.
 */

#pragma once

#include <SparkFun_GridEYE_Arduino_Library.h>
#include <Wire.h>

#include "loggable.h"

namespace dosa {

class IrGrid : public Loggable
{
   public:
    explicit IrGrid(SerialComms* s = nullptr) : Loggable(s)
    {
        Wire.begin();
        ir.begin();
    }

    [[nodiscard]] float getPixelTemp(unsigned char i)
    {
        return ir.getPixelTemperature(i);
    }

    [[nodiscard]] int16_t getPixelTempRaw(unsigned char i)
    {
        return ir.getPixelTemperatureRaw(i);
    }

    [[nodiscard]] float getDeviceTemp()
    {
        return ir.getDeviceTemperature();
    }

    [[nodiscard]] int16_t getDeviceTempRaw()
    {
        return ir.getDeviceTemperatureRaw();
    }

    [[nodiscard]] GridEYE& getGridEye()
    {
        return ir;
    }

   protected:
    bool inited = false;
    GridEYE ir{};
};

}  // namespace dosa
