local log = require "log"
local capabilities = require "st.capabilities"

local lifecycle = {}

-- Called once a device is added by the cloud and synchronised down to the hub
function lifecycle.device_added(driver, device)
    log.info(string.format("Adding new DOSA device <%s>: %s", device.device_network_id, device.label))
end

-- Called both when a device is added (but after `added`) and after a hub reboots.
function lifecycle.device_init(driver, device)
    if device == nil then
        log.error("device init called with nil device")
        return
    end

    log.info(string.format("Initialising %s device: %s", device.label, device.device_network_id))
    local dosa_id = string.sub(device.device_network_id, 1, 7)

    if string.sub(device.device_network_id, 1, 4) == "bt1:" then
        device:online()
    elseif dosa_id == "dosa-d:" then
        device:emit_event(capabilities.doorControl.door.closed())
        device:online()
    elseif dosa_id == "dosa-r:" then
        device:emit_event(capabilities.switch.switch.off())
        device:online()
    elseif dosa_id == "dosa-s:" then
        device:emit_event(capabilities.motionSensor.motion.inactive())
        device:online()
    else
        log.info(string.format("Unknown device <%s>: %s", device.device_network_id, device.label))
        device:offline()
    end

end

-- Called when a device is removed by the cloud and synchronised down to the hub
function lifecycle.device_removed(driver, device)
    log.info(string.format("Removing DOSA device <%s>: %s", device.device_network_id, device.label))
end

return lifecycle
