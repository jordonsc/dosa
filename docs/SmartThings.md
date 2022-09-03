# DOSA SmartThings Integration
This project creates SmartThings Edge drivers for the DOSA platform. By using Edge drivers, the automation within a 
SmartThings Hub will always remain and execute locally.

## Getting Started with SmartThings
* [SmartThings CLI](https://github.com/SmartThingsCommunity/smartthings-cli/releases)
* [LAN Device Tutorial](https://community.smartthings.com/t/tutorial-creating-drivers-for-lan-devices-with-smartthings-edge/229501)
* [Sample Drivers](https://github.com/SmartThingsDevelopers/SampleDrivers)

## Reference Guides
* [Edge Driver Reference](https://developer-preview.smartthings.com/docs/edge-device-drivers/reference/index.html)
* [Capability List](https://developer-preview.smartthings.com/docs/devices/capabilities/capabilities-reference/)

### Download the CLI and authenticate
[Download the SmartThings CLI](https://github.com/SmartThingsCommunity/smartthings-cli/releases) and then run 
`smartthings devices` to kick off the SmartThings login. Once done, you'll be able to see what hubs you have and what
devices are configured on each hub.


## Building 
#### Build the edge drivers
This is your primary compilation phase. The final path is the path to the `edge` folder in this repository -

    smartthings edge:drivers:package <path to src>

eg:

    smartthings edge:drivers:package smartthings

You should get an output like this:

    ───────────────────────────────────────────────────
     Driver Id    xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx 
     Name         DOSA
     Package Key  sh.shq.st-dosa                     
     Version      2022-08-31T00:42:21.012528864        
    ───────────────────────────────────────────────────

#### Create a channel for the drivers:
You'll only need to do this once. Once done, update the `ids` file with the new channel.

    smartthings edge:channels:create

#### Enroll hub in channel:
You only need to do this once _per hub_:

    smartthings edge:channels:enroll

#### Publish drivers:

    smartthings edge:drivers:publish

#### Install drivers on a hub:

    smartthings edge:drivers:install

#### To tail logs:

    source smartthings/ids
    smartthings edge:drivers:logcat $DOSA_DRIVER --hub-address <hub IP address>

#### Quick rebuild:
For every modification, you need to re-package, publish & install. This script will do all three in a single run:

    smartthings/rebuild -H <your hub ID>
