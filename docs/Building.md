Getting Started
===============
Code Layout
-----------
The project is broken into primary applications the `src/` directory, and supporting libraries:

* `door`: the primary door driver master unit that runs the door winch
* `sensor`: satellite sensor units that can connect to any master unit, contains sensors such as PIR, etc

Libraries are in the `lib/` directory and follow Arduino 2.8 library standards:

* `dosa`: common library consumed by all applications
* `door`: library for supporting the door application

Ino Files
---------
The application directories contain a `.cc` file and a `.ino` file symlinked together. The `.cc` file is there to keep 
your IDE sane, but the Arduino IDE/CLI needs to see the `.ino` file and will ignore the `.cc` file.
 
Building and Uploading
======================
You can manage the devices entirely with the [Arduino CLI](https://arduino.github.io/arduino-cli/latest/), but there is
also a quick helper bash script to run the obvious commands:

    ./dosa

Will print the usage and list connected boards. There are two steps involved in burning the application to the board:

* Compile - compiles the C++ application
* Upload  - sends that application to the Arduino board

You must always first compile:

    # Production build
    ./dosa compile door
    
    # Debug build - lowers the log-level and the application won't begin until a serial monitor is listening
    ./dosa compile-debug door

> NB: the "door" app is wifi-enabled and will require wifi credentials; see below.

The batch file will select the correct board name to send to the `arduino-cli` binary, but when you upload you need to
specify the port (you can view ports with `arduino-cli board list`).

    ./dosa upload door /dev/ttyACM0

You can do these both together with the `install` or `debug` command:

    # Compile and upload the `door` application
    ./dosa install door /dev/ttyACM0
    
    # Compile in debug mode, upload and start a monitor 
    ./dosa debug door /dev/ttyACM0

Configuring Devices
-------------------
Arduino-board devices will bring Bluetooth online when they first run, or if they fail to connect to wifi. You should
connect to the Bluetooth using a generic Bluetooth application (consider "Lightblue" for Android) and from there you
can set Characteristic values to change settings.

Sensitive values are password protected, the default password is "dosa". Use a new-line as a delimeter between values.

See `lib/dosa/src/const.h` for a list of BLE characteristics.

### Set Device Pin
Send `dosa\nnew password` to `d05a0010-e8f2-537e-4f6c-d104768a1100`

### Set Device Name
Send `dosa\nnew device name` to `d05a0010-e8f2-537e-4f6c-d104768a1002`

### Set Wifi Configuration
Send `dosa\nAP NAME\nPASSWORD` to `d05a0010-e8f2-537e-4f6c-d104768a1101`

> Once the wifi value has been updated, the device is disable Bluetooth and attempt to connect to the access point.
