local log = require "log"

local command_handlers = {
    relay = require "handlers/relay",
    sensor = require "handlers/sensor",
    door = require "handlers/door",
    grid = require "handlers/grid",
}

-- Tells the controller to power the load connection
function command_handlers.fallback(_, device, command)
    local dosa_type = string.sub(device.device_network_id, 1, 7)
    log.debug(string.format("CMD:  %s", device.device_network_id))
    log.debug(string.format("DOSA: %s", dosa_type))

    if dosa_type == "dosa-g:" then
        command_handlers.grid.exec(device, command)
    elseif dosa_type == "dosa-d:" then
        command_handlers.door.exec(device, command)
    elseif dosa_type == "dosa-r:" then
        command_handlers.relay.exec(device, command)
    elseif dosa_type == "dosa-s:" then
        command_handlers.sensor.exec(device, command)
    end
end

return command_handlers
