Getting Started
===============
Code Layout
-----------
The project is broken into primary applications the `src/` directory, and supporting libraries:

* `door`: the primary door driver master unit that runs the door winch
* `sensor`: satellite sensor units that can connect to any master unit, contains sensors such as PIR, etc

Libraries are in the `lib/` directory and follow Arduino 2.8 library standards:

* `dosa`: common library consumed by all applications
* `door`: library for supporting the door application

Ino Files
---------
The application directories contain a `.cc` file and a `.ino` file symlinked together. The `.cc` file is there to keep 
your IDE sane, but the Arduino IDE/CLI needs to see the `.ino` file and will ignore the `.cc` file.
 