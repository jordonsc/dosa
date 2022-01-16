DOSA Device Registry
====================
There are two types of devices within DOSA:
* **Master units**: Have a primary function, listens for events broadcasted by satellites
* **Satellite units**: Normally provide generic sensory information, dispatches events that master units can listen to

Device design found [here](https://drive.google.com/file/d/1iOGFvSHi1p7XgKmgojp_A2M7-Gj7CR4N/view?usp=sharing) (WIP).

> All devices are now using IoT boards and communicate via WiFi instead of BT. BT is used to configuring the boards
> only.

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
* [FRAM](https://core-electronics.com.au/adafruit-spi-non-volatile-fram-breakout-64kbit-8kbyte.html) (required for Gen II cloud support)

LED Signals:
* Green blinking: no sensors connected, ready via button press only
* Green steady: sensors connected, ready
* Green steady, blue quickly flashes: displays count of connected devices
* Green steady, blue steady: connecting to device
* Red light 1s during connect: connection failed
* Red light flashes 3 times: device disconnected
* Red steady, blue flashes: timeout during open sequence - holding for safety
* Red steady, green flashes: timeout during close sequence - holding for safety
* Red steady, green & blue alternate: motor jam - holding for safety
* All lights flash together: unknown error

Sensor Suite
------------
* Satellite unit
* Board: `arduino:samd:nano_33_iot`
* Application: `sensor`
* State: Gen I complete; IR array in development

The sensor device can host an array of sensory functions and transmit them to any master unit listening. Current
generation only supports a PIR sensor.

The sensor device is designed to be powered by batteries, thus it contains minimal components that could drain power.

Hardware components:
* [PIR sensor](https://core-electronics.com.au/pir-motion-sensor-11609.html); or
* [IR Array](https://core-electronics.com.au/sparkfun-grid-eye-infrared-array-breakout-amg8833-qwiic.html)
* 12v DC input
* Watertight housing

> The sensor suite has Gen II+ plans for solar support.
