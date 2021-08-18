Project Setup
=============

Connecting to the Arduino's
---------------------------

### IDE

Programming the Arduino's is done via the Arduino IDE. Download the [Arduino IDE](https://www.arduino.cc/en/software) to
get started.

You may use any IDE to edit code, but you'll need to use the Arduino IDE to compile and burn the binary.

### Board Driver

Once you have the IDE, you need the correct board drivers for the Nano BLE boards. Go to *Tools* > *Board* > *Board
Manager* then search for and install:

* Arduino Mbed OS Nano Boards

Select *Arduino Nano 33 BLE* from the _Board_ selection once the driver has been installed.

### Libraries

Finally, you'll need some libraries to control the Bluetooth hardware and some STL-style containers. Go to *Tools* >
*Manage Libraries* then search for and install:

* ArduinoBLE
* ArxContainer

### USB Cable

You require a USB-B Micro cable _with a data wire_ to connect to the Arduino. Many USB-A to USB-B Micro cables that ship
with hardware for the sole purpose of providing power don't include the wires for the data pins, so be aware that if the
COM port isn't showing up, you've likely got a bad cable.

When you do connect to the device with a suitable cable, the Arduino should show up in the ports selection.


Configuring Your Filesystem
---------------------------
The way the Arduino IDE locates libraries is a bit restrictive for project libraries. You'll need to symlink the Arduino
library directory to the project `src/lib` directory.

On Windows, consider using the `cmd` prompt (as Administrator) with something like this:

    cd C:\Users\USERNAME\Documents\Arduino\libraries
    C:\Users\USERNAME\Documents\Arduino\libraries>mklink /D dosa C:\Users\USERNAME\Documents\dosa\src\lib

The actual name of the directory in the libraries doesn't matter - includes should contain the filename only, the search
path is something like `Arduino\libraries\**\*`.

> CAUTION: it's very important you consider unique filenames for your library includes, because of this.

