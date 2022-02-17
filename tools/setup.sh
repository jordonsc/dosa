#!/usr/bin/env bash

app=$(python -c "import os; print(os.path.dirname(os.path.realpath(\"$0\")))")

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
arduino-cli --config-file "${cfg}" core install arduino:samd arduino:mbed_nano Croduino_Boards:Inkplate

# Install required libraries
arduino-cli --config-file "${app}" lib install ArduinoBLE WiFiNINA "Adafruit FRAM SPI" "SparkFun GridEYE AMG88 Library"

# For Python console tool
pip3 install pySerial

echo
echo "Done"
echo
