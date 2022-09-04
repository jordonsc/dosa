local log = require "log"
local capabilities = require "st.capabilities"
local comms = require "comms"

local door = {}

function door.refresh(device, device_name, _)
    log.debug(string.format("Sending ping-refresh to <%s>", device_name))

    local msg = comms.ping_refresh(device_name)
    if msg then
        device:online()
        if msg.device_state[1] == 0 then
            -- Device "OK" (dormant, door closed)
            device:emit_event(capabilities.doorControl.door.closed())
        elseif msg.device_state[1] == 1 then
            -- Device "Active" (door sequence in play)
            device:emit_event(capabilities.doorControl.door.opening())
        else
            -- Device in error state (jammed?)
            device:emit_event(capabilities.doorControl.door.open())
        end
    else
        -- no ping reply
        device:offline()
    end
end

function door.control(device, device_name, _)
    local state = device:get_latest_state("main", "doorControl", "door", "closed")

    if state == "closed" then
        log.debug(string.format("Sending trigger to <%s>", device_name))

        if comms.send_cmd(device_name, comms.create_trigger_packet(), true) then
            device:emit_event(capabilities.doorControl.door.opening())
            device:online()
        else
            device:offline()
        end
    elseif state == "open" then
        log.debug(string.format("Sending alt trigger (10) to <%s>", device_name))

        if comms.send_cmd(device_name, comms.create_alt_packet(10), true) then
            device:emit_event(capabilities.doorControl.door.closing())
            device:online()
        else
            device:offline()
        end
    else
        log.info(string.format("Cannot action device <%s> in state <%s>", device_name, state))
    end
end

function door.exec(device, command)
    local capability = command.capability
    local device_name = string.sub(device.device_network_id, 8)
    local cmd = command.command

    if capability == "doorControl" then
        door.control(device, device_name, cmd)
    elseif capability == "refresh" then
        door.refresh(device, device_name, cmd)
    end
end

return door
