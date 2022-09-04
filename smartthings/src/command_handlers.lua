local command_handlers = {
    relay = require "handlers/relay",
    sensor = require "handlers/sensor",
    door = require "handlers/door",
}

-- Tells the controller to power the load connection
function command_handlers.fallback(_, device, command)
    local dosa_type = string.sub(device.device_network_id, 1, 7)

    if string.sub(device.device_network_id, 1, 4) == "bt1:" then
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
