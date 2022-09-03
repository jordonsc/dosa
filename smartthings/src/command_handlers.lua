local log = require "log"
local capabilities = require "st.capabilities"

local command_handlers = {
    relay = require "handlers/relay",
    sensor = require "handlers/sensor",
    door = require "handlers/door",
}

function command_handlers.bt1(device, command)
    local component = command.component
    local capability = command.capability
    local cmd = command.command

    if component == "load" and capability == "switch" then
        if cmd == "on" then
            log.debug(string.format("[%s] enabling PV load", device.label))
            -- TODO
            device.profile.components["load"]:emit_event(capabilities.switch.switch.on())
        else
            log.debug(string.format("[%s] disabling PV load", device.label))
            -- TODO
            device.profile.components["load"]:emit_event(capabilities.switch.switch.off())
        end
    end

end


-- Tells the controller to power the load connection
function command_handlers.fallback(_, device, command)
    local dosa_type = string.sub(device.device_network_id, 1, 7)

    if string.sub(device.device_network_id, 1, 4) == "bt1-" then
        command_handlers.bt1(device, command)
    elseif dosa_type == "renogy-d:" then
        command_handlers.door.exec(device, command)
    elseif dosa_type == "renogy-r:" then
        command_handlers.relay.exec(device, command)
    elseif dosa_type == "renogy-s:" then
        command_handlers.sensor.exec(device, command)
    end
end

return command_handlers
