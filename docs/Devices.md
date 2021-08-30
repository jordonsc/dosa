DOSA Device Registry
====================

There are two types of devices within DOSA:
* **Master units**: BLE central; have a primary function and connect to satellite devices
* **Satellite units**: BLE peripheral; normally provide generic sensory information, advertises itself and allows 
                       master units to connect at will

Device design found [here](https://drive.google.com/file/d/1iOGFvSHi1p7XgKmgojp_A2M7-Gj7CR4N/view?usp=sharing) (WIP).

Door Driver
-----------
* Master unit
* Board: `arduino:samd:nano_33_iot`
* Application: `door`
* State: work in progress

The door driver is 12v powered system containing a motor that is responsible for driving a winch that will open and 
close a door.

Hardware components:
* [12v motor](https://core-electronics.com.au/100-1-metal-gearmotor-37dx73l-mm-12v-with-64-cpr-encoder-helical-pinion.html): drives winch
* [Motor controller](https://core-electronics.com.au/vnh5019-motor-driver-carrier.html): converts PWM signals to drive motor power, reports on current usage
* [Push-button switch w/ LED](https://core-electronics.com.au/waterproof-metal-pushbutton-with-blue-led-ring-16mm-blue-momentary.html): manual trigger of door mechanics
* [Reed switch](https://core-electronics.com.au/magnetic-door-switch-set.html): detect door fully open/closed
* 3x LEDs: visual status indicators
* 240v AC -> 12v DC power transformer
* [3.3v voltage regulator](https://core-electronics.com.au/pololu-3-3v-500ma-step-down-voltage-regulator-d24v5f3.html) (only used by motor controller, could use board power instead)

Pending:
* Physical winch
* [FRAM](https://core-electronics.com.au/adafruit-spi-non-volatile-fram-breakout-64kbit-8kbyte.html) (required for Gen II cloud support)

Sensor Suite
------------
* Satellite unit
* Board: `arduino:mbed_nano:nano33ble`
* Application: `sensor`
* State: Gen I complete

The sensor device can host an array of sensory functions and transmit them to any master unit listening. Current
generation only supports a PIR sensor.

The sensor device is designed to be powered by batteries, thus it contains minimal components that could drain power.

Hardware components:
* [PIR sensor](https://core-electronics.com.au/pir-motion-sensor-11609.html)
* 9v battery attachment
* Watertight housing

> The sensor suite has Gen II+ plans for solar support.
