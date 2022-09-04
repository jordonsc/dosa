# Renogy BT-1 Bridge
DOSA can adopt a Renogy BT-1 device as a DOSA device and integrate into the SmartThings ecosystem, however this requires
a Bluetooth-enabled Linux device to be in proximity of the BT-1 dongle.

## Prerequisites
A _Raspberry Pi_ makes for a good bridge device. Set up any Linux OS on a Pi and clone the DOSA repo, being sure to
install the pip dependencies of the `tools/` directory.

## Setup
Run the `tools/dosa-renogy.py` script to start a Renogy Bridge server. There is a SystemD service script in the
`tools/` directory you can use to provision the daemon as a system service -

    # pi@some-pi:
    git clone https://github.com/jordonsc/dosa
    sudo cp tools/renogy.service /lib/systemd/system/
    sudo systemctl enable renogy
    sudo systemctl start renogy

If you have installed the [SmartThings drivers](SmartThings.md) on a SmartThings-compatible hub, then you can run
device discovery in the same manner you would discover DOSA devices -

    Add Device -> Scan for nearby devices -> Select hub

A "Renogy BT-1" device will be discovered and added. By default, it will refresh every 30 seconds.

## Thank you
For helping figure out the Renogy BT-1 device:
 * [cyrils/renogy-bt1](https://github.com/cyrils/renogy-bt1)
 * [Olen/solar-monitor](https://github.com/Olen/solar-monitor)
 * [corbinbs/solarshed](https://github.com/corbinbs/solarshed)
 * [Rover 20A/40A Charge Controller—MODBUS Protocol](https://docs.google.com/document/d/1OSW3gluYNK8d_gSz4Bk89LMQ4ZrzjQY6/edit)

No thanks to Renogy, who provided zero help at all integrating with their devices.
