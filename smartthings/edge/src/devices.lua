local log = require "log"

local devices = { reg = {} }

function devices.infer_device(self, addr)
    for k, v in pairs(self.reg) do
        if v == addr then
            return k
        end
    end
    return nil
end

function devices.create_meta(prefix, device_id, label, profile, manufacturer)
    return {
        type = "LAN",
        device_network_id = string.format("%s:%s", prefix, device_id),
        label = label,
        profile = profile,
        manufacturer = manufacturer,
        model = "v1",
        vendor_provided_label = nil
    }
end

function devices.create_bt1_meta(device_id)
    return devices.create_meta("bt1", device_id, "Renogy BT-1", "bt1.v1", "Renogy")
end

function devices.create_dosa_sensor_meta(device_id)
    return devices.create_meta("dosa-s", device_id, device_id, "sensor.v1", "DOSA Networks")
end

function devices.create_dosa_relay_meta(device_id)
    return devices.create_meta("dosa-r", device_id, device_id, "relay.v1", "DOSA Networks")
end

function devices.create_dosa_door_meta(device_id)
    return devices.create_meta("dosa-d", device_id, device_id, "door.v1", "DOSA Networks")
end

function devices.track(self, msg, addr, type)
    if type == nil then
        local c = self.reg[msg.device_name]
        if c ~= nil then
            type = c[2]
        end
    end

    self.reg[msg.device_name] = { addr, type }
    log.trace(string.format("Track <%s:%s> -> %s (%s)", type, msg.device_name, addr, self:count()))

    return { name = msg.device_name, type = type, address = addr }
end

-- Registers the device in local cache, and if add_device is true, creates a new SmartThings device
function devices.register(self, driver, msg, addr, add_device)
    if msg.code ~= "pon" then
        return
    end

    if msg.device_type[1] >= 10 and msg.device_type[1] < 40 then
        -- Sensor device
        if add_device then
            driver:try_create_device(self.create_dosa_sensor_meta(msg.device_name))
        end
        self:track(msg, addr, "dosa-s")
    elseif msg.device_type[1] == 110 then
        -- Power Toggle
        if add_device then
            driver:try_create_device(self.create_dosa_relay_meta(msg.device_name))
        end
        self:track(msg, addr, "dosa-r")
    elseif msg.device_type[1] == 112 then
        -- Door/motor
        if add_device then
            driver:try_create_device(self.create_dosa_door_meta(msg.device_name))
        end
        self:track(msg, addr, "dosa-d")
    elseif msg.device_type[1] == 120 then
        -- Renogy BT-1 dongle
        if add_device then
            driver:try_create_device(self.create_bt1_meta(msg.device_name))
        end
        self:track(msg, addr, "bt1")
    else
        log.info(string.format("Not registering unsupported DOSA device: %s", msg.device_type[2]))
        return
    end
end

function devices.count(self)
    local count = 0
    for _ in pairs(self.reg) do
        count = count + 1
    end
    return count
end

return devices
