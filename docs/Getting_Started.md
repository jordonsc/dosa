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
----------------------
You can manage the devices entirely with the [Arduino CLI](https://arduino.github.io/arduino-cli/latest/), but there is
also a quick helper bash script to run the obvious commands:

    ./dosa

Will print the usage and list connected boards. There are two steps involved in burning the application to the board:

* Compile - compiles the C++ application
* Upload  - sends that application to the Arduino board

You must always first compile:

    ./dosa compile door

The batch file will select the correct board name to send to the `arduino-cli` binary, but when you upload you need to
specify the port (you can view ports with `arduino-cli board list`).

    ./dosa upload door /dev/ttyACM0

You can do these both together with the `install` command:

    ./dosa install door /dev/ttyACM0
