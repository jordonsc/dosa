DOSA
====
The DOSA project is a series of milestones for home automation devices. The project is closing in on the first 
milestone.

See the [DOSA Device Registry](docs/Devices.md) for details on physical devices.

Milestone 1
-----------
A central device contains a motor and winch (driver). Satellite peripherals have PIR sensors and communicate with the 
driver via Bluetooth to open a door when motion is detected.

Devices:
* Door Driver (master device)
* Sensor Suite (satellite device)

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
* [Device List](docs/Devices.md)
* [Building](docs/Building.md)
* [Troubleshooting](docs/Troubleshooting.md)

Etymology
=========
DOSA stands for 'door opening sensor automation', although plans extend beyond just a door winch (someone must have 
been hungry when picking a name).
