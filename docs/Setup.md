Project Setup
=============

Connecting to the Arduinos
--------------------------
### IDE
Programming the Arduinos is done via the Arduino IDE. Download the [Arduino IDE](https://www.arduino.cc/en/software) to
get started.

### Board Driver
Once you have the IDE, you need the correct board drivers for the Nano BLE boards. Go to *Tools* > *Board* > *Board
Manager* then search for and install *Arduino Mbed OS Nano Boards*.

Select *Arduino Nano 33 BLE* from the _Board_ selection once the driver is installed. 

### Bluetooth Library
Finally, you'll need some libraries to control the Bluetooth hardware. Go to *Tools* > *Manage Libraries* then search
for and install *ArduinoBLE*.

### USB Cable
You require a USB-B Micro cable _with a data wire_ to connect to the Arduino. Many USB-A to USB-B Micro cables that
ship with hardware for the sole purpose of providing power don't include the wires for the data pins, so be aware that
if the COM port isn't showing up, you've likely got a bad cable.

When you do connect to the device with a suitable cable, the Arduino should show up in the ports selection. 
