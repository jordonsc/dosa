DOSA Device Physical Construction
=================================

Arduino Nano 33 IoT Devices
---------------------------
### Common
*Hubs* are connections that require many wires coming in. These are typically 3.3v or GND pins, so you will need 
to create an extension allowing multiple inbound wires to these pins.

#### FRAM Header
* Install 8-pin make header on FRAM board
* Create a corresponding female header with wires ~ 8cm long, in order:
  * red
  * black
  * _skip_
  * yellow
  * blue
  * green
  * white
  * _skip_
* On the loose ends of the wires, connect female headers:
  * 2x: red, yellow
  * 3x: white, green, blue
* Mount the header to the FRAM with RED on the VCC pin, HOLD and WP should match the skipped wires

#### Arduino
Install headers on pins:
* VIN, GND
* D13 
* D10, D11, D12

Create hubs on pins:
* 3.3v
* GND

Connect the FRAM wires:
* yellow -> D13, red -> 3.3v
* white -> D10, green -> D11, blue -> D12
* black -> ground-hub
  
  
### IR Sensor
#### Sparkfun IR Grid
Use a right-angle 4x header or hard-wire 4 wires into the board:
* GND: black
* 3.3v: red
* SDA: green
* SCL: yellow

Put a 2x female header on wires:
* green
* yellow

Connect red & black to the power hubs on the Arduino.

#### Arduino
Hubs:
* 3.3v: 2x
* GND: 2x

Install headers on pins:
  * A4, A5

Connect the IR grid to pins:
* green -> A4
* yellow -> A5
  
### Sonar Sensor
#### Sonar
* Install a female 2x header to the sonar wires

#### Arduino
Hubs:
* None, you may hard-wire the FRAM directly into the Arduino's power pins.

Install headers on pins:
* TX1, RX0

Connect the sonar sensor to pins:
  * yellow -> TX1
  * white -> RX0
