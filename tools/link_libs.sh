#!/usr/bin/env bash

app=$(python -c "import os; print(os.path.dirname(os.path.realpath(\"$0\")))")

cd ${app}/..
arduino_path=$(find ~ -maxdepth 1 -name ".arduino*" | head -n 1)
samd_version=$(ls -r1 ${arduino_path}/packages/arduino/hardware/samd/ | head -n 1)
inkplate_version=$(ls -r1 ${arduino_path}/packages/Croduino_Boards/hardware/Inkplate/ | head -n 1)

echo "Detected Arduino path:           ${arduino_path}"
echo "Detected SAMD library version:   ${samd_version}"
echo "Detected Inkplate lib version:   ${inkplate_version}"
echo

# Link core libraries to the project folder -
echo "Linking Arduino libraries to project.."

# Legacy
rm -f arduino/arduino arduino/SPI arduino/Wire arduino/ArduinoBLE arduino/WiFiNINA arduino/Adafruit_FRAM_SPI \
  arduino/Adafruit_BusIO arduino/SparkFun_GridEYE_AMG88_Library arduino/Inkplate arduino/Cro_WiFi

# Remove previous links to boards & libs
rm -rf arduino/boards arduino/variants arduino/libraries

# Link board cores, their libraries & appropriate variants -
mkdir -p arduino/libraries arduino/boards/samd arduino/boards/inkplate arduino/variants
ln -s ${arduino_path}/packages/arduino/hardware/samd/${samd_version}/cores arduino/boards/samd/cores
ln -s ${arduino_path}/packages/arduino/hardware/samd/${samd_version}/libraries arduino/boards/samd/libraries
ln -s ${arduino_path}/packages/arduino/hardware/samd/${samd_version}/variants/nano_33_iot arduino/variants/nano_33_iot
ln -s ${arduino_path}/packages/Croduino_Boards/hardware/Inkplate/${inkplate_version}/cores arduino/boards/inkplate/cores
ln -s ${arduino_path}/packages/Croduino_Boards/hardware/Inkplate/${inkplate_version}/libraries arduino/boards/inkplate/libraries
ln -s ${arduino_path}/packages/Croduino_Boards/hardware/Inkplate/${inkplate_version}/variants/Inkplate arduino/variants/inkplate

# Link custom Arduino libs -
ln -s ~/Arduino/libraries/Adafruit_BusIO arduino/libraries/Adafruit_BusIO
ln -s ~/Arduino/libraries/Adafruit_FRAM_SPI arduino/libraries/Adafruit_FRAM_SPI
ln -s ~/Arduino/libraries/ArduinoBLE arduino/libraries/ArduinoBLE
ln -s ~/Arduino/libraries/Inkplate arduino/libraries/Inkplate
ln -s ~/Arduino/libraries/SparkFun_GridEYE_AMG88_Library arduino/libraries/SparkFun_GridEYE_AMG88_Library
ln -s ~/Arduino/libraries/WiFiNINA arduino/libraries/WiFiNINA

# Link project libraries to the Arduino lib folder -
echo "Linking project libraries to Arduino.."

# Prepare Arduino library directory -
mkdir -p ~/Arduino/libraries
rm -f ~/Arduino/libraries/dosa ~/Arduino/libraries/messages ~/Arduino/libraries/door ~/Arduino/libraries/sensor \
  ~/Arduino/libraries/sonar ~/Arduino/libraries/dosa_inkplate ~/Arduino/libraries/monitor ~/Arduino/libraries/comms

# (Re)link libs -
ln -s ${app}/../lib/dosa ~/Arduino/libraries/dosa
ln -s ${app}/../lib/dosa_inkplate ~/Arduino/libraries/dosa_inkplate
ln -s ${app}/../lib/comms ~/Arduino/libraries/comms
ln -s ${app}/../lib/messages ~/Arduino/libraries/messages
ln -s ${app}/../lib/door ~/Arduino/libraries/door
ln -s ${app}/../lib/sensor ~/Arduino/libraries/sensor
ln -s ${app}/../lib/sonar ~/Arduino/libraries/sonar
ln -s ${app}/../lib/monitor ~/Arduino/libraries/monitor
