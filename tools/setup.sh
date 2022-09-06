#!/usr/bin/env bash

app=$(python3 -c "import os; print(os.path.dirname(os.path.realpath(\"$0\")))")

# Set user to the dialout group so that they can access the USB
sudo usermod -a -G dialout $USER

# Check for the Arduino CLI
if [[ ! $(command -v "arduino-cli") ]]; then
  echo "Arduino CLI not found!"
  echo
  read -p "Install? " -n 1 -r INST
  echo

  if [[ ${INST} =~ ^[Yy]$ ]]; then
    echo "Installing Arduino CLI.."
    curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=~/bin sh
  else
    echo "Aborting"
    exit 1
  fi
fi

# Install boards to the CLI
cfg=${app}/arduino-cli.yaml

echo "Installing boards.."
arduino-cli --config-file "${cfg}" core update-index
# NB: issue with Adafruit BusIO on arduino:samd@1.8.11 (as of 2022-02-20)
arduino-cli --config-file "${cfg}" core install arduino:samd@1.8.11 arduino:mbed_nano Croduino_Boards:Inkplate

# Install required libraries
arduino-cli --config-file "${app}" lib install ArduinoBLE WiFiNINA Array "Adafruit FRAM SPI" \
  "SparkFun GridEYE AMG88 Library" ArduinoOTA HttpClient

# Install Inkplate library from Github
inkplate_dir=$(echo ~/Arduino/libraries/Inkplate)
if [[ -d ${inkplate_dir} ]]; then
  git -C "${inkplate_dir}" pull
else
  mkdir -p "${inkplate_dir}"
  git clone https://github.com/e-radionicacom/Inkplate-Arduino-library.git "${inkplate_dir}"
fi

# For Python applications
pip3 install -r ${app}/pip-reqs.txt

echo
echo "Done"
echo
