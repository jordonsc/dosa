local log = require "log"
local capabilities = require "st.capabilities"
local comms = require "comms"

local sensor = {}

function sensor.refresh(device, device_name, _)
    log.debug(string.format("Sending ping-refresh to <%s>", device_name))

    local msg = comms.ping_refresh(device_name)
    if msg then
        if msg.device_state[1] == 0 then
            -- Device "OK" (no motion)
            device:online()
            device:emit_event(capabilities.motionSensor.motion.inactive())
        elseif msg.device_state[1] == 1 then
            -- Device "Active" (recent motion)
            device:online()
            device:emit_event(capabilities.motionSensor.motion.active())
        else
            -- Device in error state
            device:offline()
        end
    else
        -- no ping reply
        device:offline()
    end
end

function sensor.exec(device, command)
    local capability = command.capability
    local device_name = string.sub(device.device_network_id, 8)
    local cmd = command.command

    if capability == "refresh" then
        sensor.refresh(device, device_name, cmd)
    end
end

return sensor
