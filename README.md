DOSA
====
The DOSA project is a series of milestones for home automation devices.

Milestone 1
-----------
A central device contains a motor and winch (driver). Satellite peripherals have PIR sensors and communicate with the 
driver via Bluetooth to open a door when motion is detected.

Devices:
* Door Driver (master device)
  * Arduino 33 IoT
  * 12v motor: drives winch
  * Push-button switch: manual trigger of door mechanics
  * Reed switch: detect door fully open/closed - TBD
  * BLE central
  * Wifi (dormant for Milestone 1)
  * 240v AC power transformed to 12v DC
* Sensor Suite (satellite device)
 * Arduino 33 BLE
 * PIR sensor
 * 12v battery power supply

Milestone 2
-----------
Motion Sensor satellite device now has added capability of polymer lithium ion battery and solar power.

Device changes:
* Sensor Suite (satellite device)
  * Microcontroller to be converted to MKR board
  * Add LiPo batter & solar power

Milestone 3
-----------
Master devices wifi enabled. Device components of the master device and attached satellites are sent to a cloud message
queue. FRAM memory added. Wifi configured via Bluetooth on a phone or other device. Device logic now dictated via
cloud service.

Device changes:
* _All master devices_:
 * Add FRAM chip
 
This milestone is a software-heavy phase, requires a cloud platform.

Milestone 4
-----------
Add new peripherals and automation capabilities:

New devices:
* Light Controller (master device)
  * Arduino 33 IoT
  * LED light strip
  * ?
* Camera (master device)
 * ?

Device changes:
* Sensor Suite
  * Add optional laser trip component
  * Add optional reed switch component
  * PIR sensor becomes optional
  
Sensor Suite should be plug-and-play with sensor accessories.

 
Docs
====
* [Setup](docs/Setup.md)
* [Getting Started](docs/Getting_Started.md)



