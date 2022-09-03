local log = require "log"
local comms = require "comms"
local devices = require "devices"

local function find_devices(driver, add_device)
    -- Create multicast socket for sending and receiving
    local sock = comms.create_udp_socket(true)

    -- Send ping message
    comms.mc_send(sock, comms.create_ping_packet())

    while true do
        -- Wait up to config.MC_TIMEOUT seconds for a pong message
        local packet, addr, port = sock:receivefrom()

        if packet == nil then
            -- no more responses from ping, close socket and end discovery
            sock:close()
            return
        end

        local msg = comms.handle_packet(packet)
        if msg ~= nil then
            comms.log_msg(msg, addr, port)
            devices:register(driver, msg, addr, add_device)
        end
    end
end

local discovery = {}

-- Start the discovery sequence
-- Discovery will send a single ping and wait for all responses, then return
function discovery.start(driver, _)
    log.info("Starting DOSA device discovery..")
    find_devices(driver, true)
    log.info("DOSA discovery end")
end

-- Prewarm IP cache
-- Similar to discovery, but will not add devices, instead it will cache the IP addresses of all DOSA devices
function discovery.prewarm(driver)
    log.info("Pre-warming IP cache")
    find_devices(driver, false)
    log.info("IP cache pre-warm complete")
end

return discovery
