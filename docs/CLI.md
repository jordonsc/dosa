DOSA CLI
========
There are two DOSA CLI applications:

* DOSA build tool: `./dosa`
* DOSA network tool: `./dosa-net`

DOSA Build Tool
---------------
The DOSA build tool is your primary means of compiling & uploading to boards. It will allow you to:

* Configure your local environment dependencies
* Compile a production or debug build
* Upload/install via USB
* Deploy OTA assets
* Open a serial monitor on a USB port to listen to device logs

> Run `./dosa` for a command-line reference

DOSA Network Tool
-----------------
The DOSA network tool allows you to remotely configure devices that are connected to wifi, send messages such as
triggers or OTA update requests and also run a network monitor that listens for DOSA traffic.

The network monitor can also display information such as showing the IR grid state that caused IR triggers, or the
distance deltas for ultrasonic trips. This is the best way to monitor network-level log messages, too.

> Run `./dosa-net -h` for a command-line reference.

Without any arguments it will go straight into monitor mode. You might want to consider:

    ./dosa-net -mix

which will ignore unnecessary network information and print sensor trip details.

To list active devices on the network and open up a configuration menu, consider:

    ./dosa-net -c

