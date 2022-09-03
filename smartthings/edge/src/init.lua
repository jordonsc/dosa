local Driver = require "st.driver"
local command_handlers = require "command_handlers"
local lifecycle = require "lifecycle"
local discovery = require "discovery"
local server = require "server"

-- Create the driver object
local dosa_driver = Driver("DOSA", {
    discovery = discovery.start,
    lifecycle_handlers = {
        added = lifecycle.device_added,
        init = lifecycle.device_init,
        removed = lifecycle.device_removed
    },
    capability_handlers = {
        ["fallback"] = command_handlers.fallback
    }
})

-- Pre-warm IP cache --
discovery.prewarm(dosa_driver)

-- Run the DOSA UDP server
server:start(dosa_driver)

-- Run the driver
dosa_driver:run()
