#!/usr/bin/env bash

app=$(python -c "import os; print(os.path.dirname(os.path.realpath(\"$0\")))")

cd ${app}/..
arduino_path=$(find ~ -maxdepth 1 -name ".arduino*" | head -n 1)
samd_version=$(ls -r1 ${arduino_path}/packages/arduino/hardware/samd/ | head -n 1)

echo "Detected Arduino path:           ${arduino_path}"
echo "Detected SAMD library version:   ${samd_version}"
echo

# Link core libraries to the project folder -
echo "Linking Arduino libraries to project.."

# Remove old links to the local arduino directory (used by IDE)
rm -f arduino/arduino arduino/SPI arduino/Wire arduino/ArduinoBLE arduino/WiFiNINA arduino/Adafruit_FRAM_SPI \
      arduino/Adafruit_BusIO arduino/SparkFun_GridEYE_AMG88_Library arduino/Inkplate

# Link core Arduino libs -
ln -s ${arduino_path}/packages/arduino/hardware/samd/${samd_version}/cores/arduino arduino/arduino
ln -s ${arduino_path}/packages/arduino/hardware/samd/${samd_version}/libraries/SPI arduino/SPI
ln -s ${arduino_path}/packages/arduino/hardware/samd/${samd_version}/libraries/Wire arduino/Wire

# Link custom Arduino libs -
ln -s ~/Arduino/libraries/ArduinoBLE arduino/ArduinoBLE
ln -s ~/Arduino/libraries/WiFiNINA arduino/WiFiNINA
ln -s ~/Arduino/libraries/Adafruit_FRAM_SPI arduino/Adafruit_FRAM_SPI
ln -s ~/Arduino/libraries/Adafruit_BusIO arduino/Adafruit_BusIO
ln -s ~/Arduino/libraries/SparkFun_GridEYE_AMG88_Library arduino/SparkFun_GridEYE_AMG88_Library
ln -s ~/Arduino/libraries/Inkplate arduino/Inkplate

# Link project libraries to the Arduino lib folder -
echo "Linking project libraries to Arduino.."

# Prepare Arduino library directory -
mkdir -p ~/Arduino/libraries
rm -f ~/Arduino/libraries/dosa ~/Arduino/libraries/messages ~/Arduino/libraries/door ~/Arduino/libraries/sensor \
      ~/Arduino/libraries/sonar

# (Re)link libs -
ln -s ${app}/../lib/dosa ~/Arduino/libraries/dosa
ln -s ${app}/../lib/messages ~/Arduino/libraries/messages
ln -s ${app}/../lib/door ~/Arduino/libraries/door
ln -s ${app}/../lib/sensor ~/Arduino/libraries/sensor
ln -s ${app}/../lib/sonar ~/Arduino/libraries/sonar
