local log = require "log"
local capabilities = require "st.capabilities"
local comms = require "comms"

local grid = {}

function grid.refresh(device, device_name, _)
    log.debug(string.format("Sending ping-refresh to <%s>", device_name))

    local msg = comms.req_stat(device_name)
    if msg and msg.code == "sta" and msg.status_format == 100 and msg.power_grid ~= nil then
        device:online()
        device.profile.components["main"]:emit_event(capabilities.battery.battery(msg.power_grid.battery_soc))
        device.profile.components["main"]:emit_event(capabilities.voltageMeasurement.voltage(msg.power_grid.battery_voltage))
        device.profile.components["main"]:emit_event(capabilities.temperatureMeasurement.temperature({value = msg.power_grid.battery_temperature, unit = 'C'}))

        device.profile.components["pv"]:emit_event(capabilities.voltageMeasurement.voltage(msg.power_grid.pv_voltage))
        device.profile.components["pv"]:emit_event(capabilities.powerMeter.power(msg.power_grid.pv_power))
        device.profile.components["pv"]:emit_event(capabilities.energyMeter.energy(msg.power_grid.pv_produced))

        if msg.power_grid.load_state then
            device.profile.components["load"]:emit_event(capabilities.switch.switch.on())
        else
            device.profile.components["load"]:emit_event(capabilities.switch.switch.off())
        end
        device.profile.components["load"]:emit_event(capabilities.powerMeter.power(msg.power_grid.load_power))
        device.profile.components["load"]:emit_event(capabilities.energyMeter.energy(msg.power_grid.load_consumed))
        device.profile.components["load"]:emit_event(capabilities.temperatureMeasurement.temperature({value=msg.power_grid.controller_temperature, unit='C'}))
    else
        -- no status reply/error
        device:offline()
    end
end

function grid.switch_load(device, device_name, cmd)
    if cmd == "on" then
        log.debug(string.format("[%s] enabling PV load", device_name))
        -- TODO
        device.profile.components["load"]:emit_event(capabilities.switch.switch.on())
    else
        log.debug(string.format("[%s] disabling PV load", device_name))
        -- TODO
        device.profile.components["load"]:emit_event(capabilities.switch.switch.off())
    end
end

function grid.exec(device, command)
    local component = command.component
    local capability = command.capability
    local cmd = command.command
    local device_name = string.sub(device.device_network_id, 5)

    log.debug(string.format("GRID exec: %s.%s.%s (%s)", component, capability, cmd, device_name))

    if component == "main" then
        if capability == "refresh" then

            grid.refresh(device, device_name, cmd)

        end
    elseif component == "load" then
        if capability == "switch" then
            grid.switch_load(device, device_name, cmd)
        end
    end
end

return grid
