local log = require "log"
local config = require "config"
local math = require "math"
local os = require "os"
local socket = require "socket"
local devices = require "devices"

local comms = {}

-- Bring up the comms, randomising the packet IDs
function comms.init()
    math.randomseed(os.time())
end

function comms.type_code_to_string(code)
    if code == 0 then
        return "Unknown"
    elseif code == 1 then
        return "Monitor"
    elseif code == 2 then
        return "Utility"
    elseif code == 3 then
        return "Alarm"
    elseif code == 10 then
        return "IR Passive"
    elseif code == 11 then
        return "IR Active"
    elseif code == 12 then
        return "Optical"
    elseif code == 20 then
        return "Sonar"
    elseif code == 40 then
        return "Button"
    elseif code == 41 then
        return "Toggle"
    elseif code == 110 then
        return "Power Toggle"
    elseif code == 112 then
        return "Motor"
    elseif code == 113 then
        return "Light"
    elseif code == 120 then
        return "Power Grid"
    elseif code == 121 then
        return "Battery"
    elseif code == 122 then
        return "Solar Panel"
    elseif code == 123 then
        return "Power Monitor"
    end
end

function comms.state_code_to_string(code)
    if code == 0 then
        return "OK"
    elseif code == 1 then
        return "Active"
    elseif code == 10 then
        return "Minor Fault"
    elseif code == 11 then
        return "Major Fault"
    elseif code == 12 then
        return "Critical"
    end
end

function comms.log_msg(msg, addr, port)
    if msg.code == "pon" then
        log.debug(string.format("[COMMS] %s:%s (%s): %s // %s - %s", addr, port, msg.device_name, msg.code, msg.device_type[2], msg.device_state[2]))
    else
        log.debug(string.format("[COMMS] %s:%s (%s): %s", addr, port, msg.device_name, msg.code))
    end
end

function comms.handle_packet(packet)
    if string.len(packet) < config.DOSA_BASE_PAYLOAD_SIZE then
        log.warning("Received non-DOSA packet on DOSA multicast group")
        return nil
    end

    local msg = {}
    msg.code = string.sub(packet, 3, 5)
    msg.device_name = string.gsub(string.sub(packet, 7, 27), string.char(0x00), "")
    msg.size = string.unpack("<I2", string.sub(packet, 6, 7))

    if msg.code == "pon" then
        -- Decode 'pong' message
        if msg.size ~= 29 then
            log.error(string.format("Pong message size mismatch: %s != 29", msg.size))
            msg.code = "xxx"
        else
            local type_code = string.byte(packet, 28)
            local state_code = string.byte(packet, 29)
            msg.device_type = { type_code, comms.type_code_to_string(type_code) }
            msg.device_state = { state_code, comms.state_code_to_string(state_code) }
        end
    elseif msg.code == "sta" then
        -- Decode 'status' message
        msg.status_format = string.unpack("<I2", string.sub(packet, 28, 29))
        msg.status_payload = string.sub(packet, 30, msg.size)
        return comms.decode_stat(msg)
    end

    return msg
end

function comms.decode_stat(msg)
    if msg.status_format == 0 then
        -- "Status Only" format
        msg.status = msg.status_payload[1]

    elseif msg.status_format == 100 then
        -- "Power Grid" format
        msg.power_grid = {}

        msg.power_grid.battery_soc = string.byte(string.sub(msg.status_payload, 1, 1))
        msg.power_grid.battery_voltage = string.unpack("<I2", string.sub(msg.status_payload, 2, 3)) / 10
        msg.power_grid.battery_temperature = string.unpack("<i2", string.sub(msg.status_payload, 4, 5))

        msg.power_grid.pv_power = string.unpack("<I2", string.sub(msg.status_payload, 6, 7))
        msg.power_grid.pv_voltage = string.unpack("<I2", string.sub(msg.status_payload, 8, 9)) / 10
        msg.power_grid.pv_produced = string.unpack("<I2", string.sub(msg.status_payload, 10, 11))

        msg.power_grid.load_state = string.byte(string.sub(msg.status_payload, 12, 12)) == 1
        msg.power_grid.load_power = string.unpack("<I2", string.sub(msg.status_payload, 13, 14))
        msg.power_grid.load_consumed = string.unpack("<I2", string.sub(msg.status_payload, 15, 16))

        msg.power_grid.controller_temperature = string.unpack("<i2", string.sub(msg.status_payload, 17, 18))
    end

    return msg
end

-- Create the base part of a DOSA packet
function comms.create_dosa_packet(cmd, aux_size)
    return string.char(math.random(0, 255), math.random(0, 255)) .. cmd ..
            string.pack("<I2", config.DOSA_BASE_PAYLOAD_SIZE + aux_size) .. config.DEVICE_NAME ..
            string.rep(string.char(0x00), 20 - string.len(config.DEVICE_NAME))
end


-- Create a DOSA ping payload
function comms.create_ping_packet()
    log.debug("[COMMS] <pin>")
    return comms.create_dosa_packet("pin", 0)
end

-- Create a DOSA req-stat payload
function comms.create_req_stat_packet()
    log.debug("[COMMS] <req>")
    return comms.create_dosa_packet("req", 0)
end

-- Create a DOSA trigger payload
function comms.create_trigger_packet()
    log.debug("[COMMS] <trg>")
    return comms.create_dosa_packet("trg", 65) .. string.char(0x02) .. string.rep(string.char(0x00), 64)
end

-- Create a DOSA alt payload
function comms.create_alt_packet(val)
    log.debug("[COMMS] <alt>")
    return comms.create_dosa_packet("alt", 2) .. string.pack("<I2", val)
end

-- Create a UDP socket
function comms.create_udp_socket(listen)
    local udp_sock = socket.udp()

    if listen then
        udp_sock:setoption("reuseaddr", true)

        local ok, err = udp_sock:setsockname("*", 0)
        if not ok then
            log.error("Failed to bind UDP socket: " .. err)
        else
            log.debug(string.format("UDP server running on %s:%s", udp_sock:getsockname()))
        end

        udp_sock:settimeout(config.MC_TIMEOUT)
    end

    return udp_sock
end

-- Opens a socket to send a single packet, then closes
function comms.mc_send(sock, packet)
    sock:sendto(packet, config.MC_ADDRESS, config.MC_PORT)
end

function comms.send_cmd(device_name, packet, ack)
    local addr = devices.reg[device_name][1]
    if addr == nil then
        log.error(string.format("Device <%s> not registered, ignoring cmd dispatch: ", device_name))
        return
    end
    log.trace(string.format("Dispatch to %s at %s", device_name, addr))

    local sock = comms.create_udp_socket(ack)
    sock:sendto(packet, addr, config.MC_PORT)

    if ack then
        while true do
            local resp = sock:receivefrom()
            if resp == nil then
                -- timeout waiting for ack
                log.error(string.format("No ack from %s (%s)", device_name, addr))
                sock:close()
                return false
            end

            local msg = comms.handle_packet(resp)
            if msg ~= nil and msg.code == "ack" then
                -- ack found, return positive result
                -- NB: not checking msg ID, BUT, this is a random sending port, so would be extremely unlikely it's
                --     not the right message
                log.trace(string.format("%s (%s): ack", device_name, addr))
                sock:close()
                return true
            end
        end
    end

    sock:close()
    return nil
end

function comms.ping_refresh(device_name)
    local addr = devices.reg[device_name][1]
    if addr == nil then
        log.error(string.format("Device <%s> not registered, ignoring refresh dispatch: ", device_name))
        return
    end
    log.trace(string.format("Ping refresh to %s at %s", device_name, addr))

    local sock = comms.create_udp_socket(true)
    sock:sendto(comms.create_ping_packet(), addr, config.MC_PORT)

    while true do
        local resp = sock:receivefrom()
        if resp == nil then
            -- timeout waiting for pong
            log.error(string.format("No pong from %s (%s)", device_name, addr))
            sock:close()
            return nil
        end

        local msg = comms.handle_packet(resp)
        if msg ~= nil and msg.code == "pon" then
            log.trace(string.format("%s (%s): pon", device_name, addr))
            sock:close()
            return msg
        end
    end
end

function comms.req_stat(device_name)
    local addr = devices.reg[device_name][1]
    if addr == nil then
        log.error(string.format("Device <%s> not registered, ignoring req-stat dispatch: ", device_name))
        return
    end
    log.trace(string.format("Req-stat to %s at %s", device_name, addr))

    local sock = comms.create_udp_socket(true)
    sock:sendto(comms.create_req_stat_packet(), addr, config.MC_PORT)

    while true do
        local resp = sock:receivefrom()
        if resp == nil then
            -- timeout waiting for status update
            log.error(string.format("No status from %s (%s)", device_name, addr))
            sock:close()
            return nil
        end

        local msg = comms.handle_packet(resp)
        if msg ~= nil and msg.code == "sta" then
            log.trace(string.format("%s (%s): sta", device_name, addr))
            sock:close()
            return msg
        end
    end
end

return comms
