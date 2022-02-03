#pragma once

#include <dosa.h>

#include "door_lights.h"
#include "door_winch.h"

#define PIN_SWITCH_DOOR 9

namespace dosa::door {

class DoorContainer : public Container
{
   public:
    DoorContainer() : Container(), door_winch(&serial, settings), door_switch(PIN_SWITCH_DOOR, true) {}

    DoorWinch& getDoorWinch()
    {
        return door_winch;
    }

    DoorLights& getDoorLights()
    {
        return door_lights;
    }

    Switch& getDoorSwitch()
    {
        return door_switch;
    }

   protected:
    DoorWinch door_winch;
    DoorLights door_lights;
    Switch door_switch;
};

}  // namespace dosa::door
