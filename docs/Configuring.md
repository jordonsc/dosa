Configuring DOSA Devices
========================
When a device first comes online, or if there is an issue connecting to wifi, it will enter Bluetooth mode whereby you
can connect and set config options.

> The default Bluetooth password is 'dosa'.

Once wifi is established, you can send more advanced configuration such as calibration data via the DOSA Network Tool:

    ./dosa-net -c
    
DOSA device configuration is stored on an FRAM chip, thus is persistent even after the device loses power.
    
Bluetooth Configuration
-----------------------
You'll want a BT inspection tool on your mobile device to help with this, 
"[Lightblue](https://play.google.com/store/apps/details?id=com.punchthrough.lightblueexplorer&hl=en_AU&gl=US)" is a 
great app for Android that will do exactly what you need.

The format for sending data to the device is always new-line delimited, and proceeding with the current DOSA password.

### To Update The BT Password
Characteristic: `d05a0010-e8f2-537e-4f6c-d104768a1100`
Syntax: `current password`, `new password`
Example payload: `dosa\nnew_password`

### To Update Device Name
Characteristic: `d05a0010-e8f2-537e-4f6c-d104768a1002`
Syntax: `current password`, `new device name`
Example payload: `dosa\nSuperSensor`

### To Set Wifi Details
Characteristic: `d05a0010-e8f2-537e-4f6c-d104768a1101`
Syntax: `current password`, `wifi SSID`, `wifi password`
Example payload: `dosa\nHome Wifi\nsupersecretpassword`
