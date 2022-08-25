#pragma once

#include <dosa.h>
#include <dosa_sonar.h>

#include "door_lights.h"
#include "door_winch.h"

#define PIN_SWITCH_DOOR 9
#define PIN_SWITCH_ALT 8

namespace dosa {

class DoorContainer : public Container
{
   public:
    DoorContainer()
        : Container(),
          door_winch(&serial, settings, sonar),
          door_switch(PIN_SWITCH_DOOR, true),
          recovery_switch(PIN_SWITCH_ALT, true)
    {}

    [[nodiscard]] DoorWinch& getDoorWinch()
    {
        return door_winch;
    }

    [[nodiscard]] DoorLights& getDoorLights()
    {
        return door_lights;
    }

    [[nodiscard]] Switch& getDoorSwitch()
    {
        return door_switch;
    }

    [[nodiscard]] Switch& getRecoverySwitch()
    {
        return recovery_switch;
    }

    [[nodiscard]] Sonar& getSonar()
    {
        return sonar;
    }

   protected:
    Sonar sonar;
    DoorWinch door_winch;
    DoorLights door_lights;
    Switch door_switch;
    Switch recovery_switch;
};

}  // namespace dosa
