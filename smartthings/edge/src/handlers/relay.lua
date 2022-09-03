local log = require "log"
local capabilities = require "st.capabilities"
local comms = require "comms"

local relay = {}

function relay.switch(device, device_name, cmd)
    if cmd == "on" then
        log.debug(string.format("Sending trigger to <%s>", device_name))

        if comms.send_cmd(device_name, comms.create_trigger_packet(), true) then
            device:emit_event(capabilities.switch.switch.on())
            device:online()
        else
            device:offline()
        end
    else
        log.debug(string.format("Sending alt trigger to <%s>", device_name))

        if comms.send_cmd(device_name, comms.create_alt_packet(0), true) then
            device:emit_event(capabilities.switch.switch.off())
            device:online()
        else
            device:offline()
        end
    end
end

function relay.refresh(device, device_name, _)
    log.debug(string.format("Sending ping-refresh to <%s>", device_name))

    local msg = comms.ping_refresh(device_name)
    if msg then
        if msg.device_state[1] == 0 then
            -- Device "OK" (switch off)
            device:online()
            device:emit_event(capabilities.switch.switch.off())
        elseif msg.device_state[1] == 1 then
            -- Device "Active" (switch on)
            device:online()
            device:emit_event(capabilities.switch.switch.on())
        else
            -- Device in error state
            device:offline()
        end
    else
        -- no ping reply
        device:offline()
    end
end

function relay.exec(device, command)
    local capability = command.capability
    local device_name = string.sub(device.device_network_id, 8)
    local cmd = command.command

    if capability == "switch" then
        relay.switch(device, device_name, cmd)
    elseif capability == "refresh" then
        relay.refresh(device, device_name, cmd)
    end
end

return relay
