DOSA
====
DOSA is a set of localised automation devices, designed to communicated on a closed network without cloud dependencies.
Devices include sensors, physical motor drivers and display monitors. 

Docs
====
* [Setup](docs/Setup.md)
* [Building](docs/Building.md)
* [Configuring Devices](docs/Configuring.md)
* [Command Line Interface](docs/CLI.md)
* [Over-The-Air updates](docs/OTA.md)
* [Guide to using graphics](docs/Graphics.md)
* [Troubleshooting](docs/Troubleshooting.md)

Devices
=======
All devices use wifi to communicate with each other. The wifi requires UDP multicasting enabled, but does not require
an internet connection.

If wifi fails to connect, or on first run, the device will fallback into Bluetooth mode allowing it to be configured
by proximity.

> The default Bluetooth password is `dosa` but can be changed by Bluetooth or wifi commands.

Basic configuration options exist over the BLE interface. You can set the wifi AP, device name and change the BT
password, however device calibration must be done over wifi.

Common Hardware
---------------
All DOSA devices using the `arduino:samd:nano_33_iot` board also sport an FRAM chip to store configuration:
* [FRAM breakout](https://core-electronics.com.au/adafruit-spi-non-volatile-fram-breakout-64kbit-8kbyte.html)
 
Door Driver
-----------
* Board: `arduino:samd:nano_33_iot`
* Application: `door`

The door driver is 12v powered system containing a motor that is responsible for driving a winch that will open and 
close a door.

Hardware components:
* [12v motor](https://core-electronics.com.au/100-1-metal-gearmotor-37dx73l-mm-12v-with-64-cpr-encoder-helical-pinion.html): drives winch
* [Motor controller](https://core-electronics.com.au/vnh5019-motor-driver-carrier.html): converts PWM signals to drive motor power, reports on current usage
* [Push-button switch w/ LED](https://core-electronics.com.au/waterproof-metal-pushbutton-with-blue-led-ring-16mm-blue-momentary.html): manual trigger of door mechanics
* [Reed switch](https://core-electronics.com.au/magnetic-door-switch-set.html): detect door fully open/closed
* [3.3v voltage regulator](https://core-electronics.com.au/pololu-3-3v-500ma-step-down-voltage-regulator-d24v5f3.html) (only used by motor controller, could use board power instead)

> Device design found [here](https://drive.google.com/file/d/1iOGFvSHi1p7XgKmgojp_A2M7-Gj7CR4N/view?usp=sharing)

PIR Sensor
----------
* Board: `arduino:samd:nano_33_iot`
* Application: `sensor`

The sensor device uses a passive IR grid and software algorithms to calibrate motion. It excels indoors in cool 
climates, but has issues in warm environments near body temperature and is subject to false positives and may require
fine calibration.

Hardware components:
* [IR Array](https://core-electronics.com.au/sparkfun-grid-eye-infrared-array-breakout-amg8833-qwiic.html)
* 12v DC input
* Watertight housing

Sonar Sensor
------------
* Board: `arduino:samd:nano_33_iot`
* Application: `sonar`

The sonar sensor is an ultrasonic version of the infrared sensor. It swaps out the IR grid in favour of an ultrasonic
ranging sonar and does not struggle with thermal issues making it far more reliable, easier to configure and still
equally weather-safe.

Hardware components:
* [Ultrasonic Sonar](https://core-electronics.com.au/large-ultrasonic-sonar-sensor-with-horn-and-uart-output.html)
* 12v DC input
* Watertight housing

Display Monitor
---------------
* Board: `Croduino_Boards:Inkplate:Inkplate6`
* Application: `monitor`

The monitor is an Inkplate (recycled Kindle) used to monitor and display the state of all DOSA devices detected on the
network. It can display visual alerts if a device or network fails, send user-initiated triggers and also contains a
sound board for audio alerts.

Hardware components:
* [Inkplate 6](https://core-electronics.com.au/inkplate-6.html)
* [Sound board](https://core-electronics.com.au/adafruit-audio-fx-sound-board-wav-ogg-trigger-with-16mb-flash.html)


Etymology
=========
DOSA stands for 'door opening sensor automation'.
