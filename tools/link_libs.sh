#!/usr/bin/env bash

app=$(python -c "import os; print(os.path.dirname(os.path.realpath(\"$0\")))")

cd ${app}/..

# Link core libraries to the project folder -
echo "Linking Arduino libraries to project.."
rm -f arduino/arduino
rm -f arduino/SPI
rm -f arduino/Wire
rm -f arduino/ArduinoBLE
rm -f arduino/WiFiNINA
rm -f arduino/Adafruit_FRAM_SPI
rm -f arduino/Adafruit_BusIO
rm -f arduino/SparkFun_GridEYE_AMG88_Library
ln -s ~/.arduino15/packages/arduino/hardware/samd/1.8.11/cores/arduino arduino/arduino
ln -s ~/.arduino15/packages/arduino/hardware/samd/1.8.11/libraries/SPI arduino/SPI
ln -s ~/.arduino15/packages/arduino/hardware/samd/1.8.11/libraries/Wire arduino/Wire
ln -s ~/Arduino/libraries/ArduinoBLE arduino/ArduinoBLE
ln -s ~/Arduino/libraries/WiFiNINA arduino/WiFiNINA
ln -s ~/Arduino/libraries/Adafruit_FRAM_SPI arduino/Adafruit_FRAM_SPI
ln -s ~/Arduino/libraries/Adafruit_BusIO arduino/Adafruit_BusIO
ln -s ~/Arduino/libraries/SparkFun_GridEYE_AMG88_Library arduino/SparkFun_GridEYE_AMG88_Library

# Link project libraries to the Arduino lib folder -
echo "Linking project libraries to Arduino.."
mkdir -p ~/Arduino
rm -f ~/Arduino/libraries/dosa ~/Arduino/libraries/messages ~/Arduino/libraries/door ~/Arduino/libraries/sensor
ln -s ${app}/../lib/dosa ~/Arduino/libraries/dosa
ln -s ${app}/../lib/messages ~/Arduino/libraries/messages
ln -s ${app}/../lib/door ~/Arduino/libraries/door
ln -s ${app}/../lib/sensor ~/Arduino/libraries/sensor
ln -s ${app}/../lib/sonar ~/Arduino/libraries/sonar
