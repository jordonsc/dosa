local capabilities = require "st.capabilities"
local log = require "log"
local comms = require "comms"
local config = require "config"
local devices = require "devices"
local socket = require("cosock").socket

local hub_server = { is_running = false }

function hub_server.get_device(self, meta)
    if self.driver == nil or meta.type == nil or meta.name == nil then
        return nil
    end

    for _, device in pairs(self.driver:get_devices()) do
        if device.device_network_id == meta.type .. ":" .. meta.name then
            return device
        end
    end

    return nil
end

function hub_server.tick(self)
    if self.is_running == false then
        log.error("DOSA hub server is not running!")
        self.is_running = nil -- to prevent spamming the error message
        return
    elseif self.is_running == nil then
        return
    end

    local packet, addr, port = self.mc_sock:receivefrom()
    if packet == nil then
        return
    end

    local msg = comms.handle_packet(packet)
    if msg == nil then
        return
    end

    comms.log_msg(msg, addr, port)
    local dvc_meta = devices:track(msg, addr, nil)
    if dvc_meta.type == nil then
        return
    end

    local device = self:get_device(dvc_meta)
    if device == nil then
        return
    end

    device:online()

    if dvc_meta.type == "bt1" then
        --
    elseif dvc_meta.type == "dosa-d" then
        if msg.code == "bgn" then
            device:emit_event(capabilities.doorControl.door.opening())
        elseif msg.code == "end" then
            device:emit_event(capabilities.doorControl.door.closed())
        end
    elseif dvc_meta.type == "dosa-r" then
        if msg.code == "bgn" then
            device:emit_event(capabilities.switch.switch.on())
        elseif msg.code == "end" then
            device:emit_event(capabilities.switch.switch.off())
        end
    elseif dvc_meta.type == "dosa-s" then
        if msg.code == "bgn" or msg.code == "trg" then
            device:emit_event(capabilities.motionSensor.motion.active())
            self.driver:call_with_delay(10, function()
                device:emit_event(capabilities.motionSensor.motion.inactive())
            end)

        elseif msg.code == "end" then
            device:emit_event(capabilities.motionSensor.motion.inactive())
        end
    else
        log.debug(string.format("SVR: unknown dosa type: '%s'", dvc_meta.type))
    end
end

function hub_server.start(self, driver)
    if driver == nil then
        log.error("No driver passed to DOSA hub server")
        return
    end

    log.debug("DOSA hub server spawning")

    self.driver = driver
    self.is_running = true

    -- Multicast socket
    self.mc_sock = socket.udp()
    self.mc_sock:setoption("ip-multicast-loop", false)
    self.mc_sock:setoption("reuseaddr", true)
    self.mc_sock:setoption("reuseport", true)
    local ok, err = self.mc_sock:setsockname(config.MC_ADDRESS, config.MC_PORT)
    if not ok then
        log.error("Failed to bind multicast server: " .. err)
        self.is_running = false
        return
    end

    log.debug(string.format("DOSA multicast server running on %s:%s", self.mc_sock:getsockname()))
    self.mc_sock:setoption("ip-add-membership", { multiaddr = config.MC_ADDRESS, interface = "0.0.0.0" })

    self.driver:register_channel_handler(self.mc_sock, function()
        self:tick()
    end)

    log.info("DOSA hub server started")
end

function hub_server.shutdown(self)
    if not self.is_running then
        return
    end

    self.driver:unregister_channel_handler(self.mc_sock)
    self.mc_sock:close()

    self.is_running = false
    log.info("DOSA hub server shutdown")
end

return hub_server
