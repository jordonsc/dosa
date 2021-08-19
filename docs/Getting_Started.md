Getting Started
===============
Code Layout
-----------
The project is broken into two binaries and a library, all in the `src/` directory:

* `main`: binary for the primary drive unit that runs the door winch
* `sensor`: binary for satellite PIR sensor units that send "open" signals to the `main` unit
* `lib`: common library between the projects

The binary directories contain a `.cc` file and a `.ino` file symlinked together. The `.cc` file is there to keep your
IDE sane, but the Arduino IDE needs to see the `.ino` file and will ignore the `.cc` file.
 