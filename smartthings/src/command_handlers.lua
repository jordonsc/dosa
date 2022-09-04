local log = require "log"

local command_handlers = {
    relay = require "handlers/relay",
    sensor = require "handlers/sensor",
    door = require "handlers/door",
    bt1 = require "handlers/bt1",
}

-- Tells the controller to power the load connection
function command_handlers.fallback(_, device, command)
    local dosa_type = string.sub(device.device_network_id, 1, 7)
    log.debug(string.format("CMD:  %s", device.device_network_id))
    log.debug(string.format("DOSA: %s", dosa_type))

    if string.sub(device.device_network_id, 1, 4) == "bt1:" then
        log.debug(string.format("exec BT-1 command"))
        command_handlers.bt1.exec(device, command)
    elseif dosa_type == "dosa-d:" then
        command_handlers.door.exec(device, command)
    elseif dosa_type == "dosa-r:" then
        command_handlers.relay.exec(device, command)
    elseif dosa_type == "dosa-s:" then
        command_handlers.sensor.exec(device, command)
    end
end

return command_handlers
