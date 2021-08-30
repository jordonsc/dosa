#!/usr/bin/env bash

app=$(python -c "import os; print(os.path.dirname(os.path.realpath(\"$0\")))")

cd ${app}/..

# Link core libraries to the project folder -
echo "Linking core libraries to project.."
rm -f arduino/arduino
rm -f arduino/ArduinoBLE
ln -s ~/.arduino15/packages/arduino/hardware/samd/1.8.11/cores/arduino arduino/arduino
ln -s ~/Arduino/libraries/ArduinoBLE arduino/ArduinoBLE

# Link project libraries to the Arduino lib folder -
echo "Linking project libraries to Arduino.."
mkdir -p ~/Arduino
rm ~/Arduino/libraries/dosa ~/Arduino/libraries/door
ln -s ${app}/../lib/dosa ~/Arduino/libraries/dosa
ln -s ${app}/../lib/door ~/Arduino/libraries/door
