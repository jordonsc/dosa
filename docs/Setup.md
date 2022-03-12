Project Setup
=============
Requirements
------------
> The setup has been designed, and only tested on, a Ubuntu platform. While there is nothing stopping you using 
> practically any other OS, Windows included, you'll probably need to port the configuration scripts first.

Insta-setup
-----------
Some helper scripts will perform all required setup for you. These are tested on Ubuntu, use at your own risk.

* Run `tools/setup.sh` to install CLI, boards and dependencies.
* Run `tools/link_libs.sh` to link libraries to the required folders for IDE and compiler both.

Manual Setup
------------
> There are quite a lot of libraries that need to be configured. It's advised to avoid attempting a manual setup.

### Arduino CLI
Install the Arduino CLI, the `curl` command will install the CLI to the `~/bin` directory. Change `BINDIR` if you want
to install elsewhere:
 
    curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=~/bin sh

> See also: [Install Page](https://arduino.github.io/arduino-cli/latest/installation/)

Install required boards:
    
    core install arduino:samd arduino:mbed_nano
    
You can view the installed boards with the `core list` command:

    arduino-cli core list

Install required libraries:

    arduino-cli lib install ArduinoBLE


### USB Cable

You require a USB-B Micro cable _with a data wire_ to connect to the Arduino. Many USB-A to USB-B Micro cables that ship
with hardware for the sole purpose of providing power don't include the wires for the data pins, so be aware that if the
COM port isn't showing up, you've likely got a bad cable.

When you do connect to the device with a suitable cable, the Arduino should show up in the ports selection.


### Configuring Your Filesystem
The way the Arduino IDE locates libraries is a bit restrictive for project libraries. You'll need to symlink the Arduino
library directory to the project `lib/dosa` directory.

The actual name of the directory in the libraries doesn't matter - includes should contain the filename only, the search
path is something like `Arduino\libraries\**\*`.

> It's important you consider globally unique filenames for your library headers.

**Linux**

    sudo ln -s ~/PROJECT-PATH/src/lib /root/Arduino/libraries/dosa

**Windows**
On Windows, consider using the `cmd` prompt (as Administrator) with something like this:

    cd C:\Users\USERNAME\Documents\Arduino\libraries
    C:\Users\USERNAME\Documents\Arduino\libraries>mklink /D dosa C:\Users\USERNAME\Documents\PROJECT-PATH\lib\dosa

### IDE Reverse Symlinks
If you're using an IDE that wants to be able to see the Arduino library headers, you can create some symlinks in the
`arduino/` directory (which will be git ignored). There is a Bazel build file there pre-configured.

    # Update this path with the location of your Arduino IDE installation -
    ln -s ~/bin/arduino-1.8.15/hardware/arduino/avr/cores/arduino arduino/arduino
    
    # This can only be linked after you've downloaded the libraries in the IDE
    ln -s ~/Arduino/libraries/ArduinoBLE arduino/ArduinoBLE

> NB: The `tools/link_libs.sh` script will symlink all required libraries.
