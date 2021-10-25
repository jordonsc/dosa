#!/usr/bin/env bash

app=$(python -c "import os; print(os.path.dirname(os.path.realpath(\"$0\")))")
cd ${app}/..

# Update this list to match board to application -
function getFqbn() {
  case "$1" in
  "door")
    echo "arduino:samd:nano_33_iot"
    ;;
  "sensor")
    echo "arduino:mbed_nano:nano33ble"
    ;;
  *) ;;

  esac
}

function syntax() {
  echo "Usage: "
  echo "  dosa [COMMAND] [APPLICATION] [PORT]"
  echo
  echo "Commands:"
  echo "  compile        :  Compiles the application, does not upload"
  echo "  compile-debug  :  Compiles the application in debug mode, does not upload"
  echo "  upload         :  Uploads the last compiled application"
  echo "  install        :  Compiles application, uploads if compile is successful"
  echo "  install-debug  :  Compiles application in debug mode, uploads if compile is successful"
  echo "  monitor        :  Opens a serial monitor to provided port (wrapper for screen)"
  echo
  echo "Applications:"
  echo "  door           : Master unit for door driver"
  echo "  sensor         : Satellite unit for sensory suite"
  echo
  echo "Connected boards & ports:"
  arduino-cli board list | awk '{ print "  " $0 }'
  exit 1
}

function validatePort() {
  if [[ -z "$1" ]]; then
    echo "Port name required, aborting"
    exit 3
  fi

  if [[ ! -c "$1" ]]; then
    echo "Port assignment invalid: $1"
    echo
    echo "Connected boards:"
    echo
    arduino-cli board list
    exit 4
  fi
}

if [[ $# -lt 2 ]]; then
  syntax
fi

function validateApp() {
  if [ -z "$1" ]; then
    echo "Invalid application, aborting"
    exit 2
  fi
}

function getBuildFlags() {
  if [ "$1" == "debug" ]; then
    echo -n "-DDOSA_DEBUG=1 "
  fi

  if [ -n "${DOSA_NET_SSID}" ]; then
    echo -n "-DDOSA_NET_SSID=$(echo -n ${DOSA_NET_SSID} | base64) "
  fi

  if [ -n "${DOSA_NET_PW}" ]; then
    echo -n "-DDOSA_NET_PW=$(echo -n ${DOSA_NET_PW} | base64) "
  fi
}

fqbn=$(getFqbn $2)

case $1 in
"compile")
  validateApp $fqbn
  echo "Compile '$2' against ${fqbn}.."
  arduino-cli compile -b ${fqbn} --build-property "compiler.cpp.extra_flags=$(getBuildFlags)" "src/$2"
  ;;
"compile-debug")
  validateApp $fqbn
  echo "[DEBUG] Compile '$2' against ${fqbn}.."
  arduino-cli compile -b ${fqbn} --build-property "compiler.cpp.extra_flags=$(getBuildFlags debug)" "src/$2"
  ;;
"upload")
  validateApp $fqbn
  validatePort $3
  echo "Uploading $2 to board on port $3.."
  arduino-cli upload -b ${fqbn} -p $3 "src/$2"
  ;;
"install")
  validateApp $fqbn
  validatePort $3
  echo "Compile $1 against ${fqbn}.."
  arduino-cli compile -b ${fqbn} --build-property "compiler.cpp.extra_flags=$(getBuildFlags)" "src/$2"
  if [[ $? -eq 0 ]]; then
    echo "Uploading $1 to board on port $3.."
    arduino-cli upload -b ${fqbn} -p $3 "src/$2"
  else
    echo
    echo "Compile error, not uploading"
  fi
  ;;
"install-debug")
  validateApp $fqbn
  validatePort $3
  echo "[DEBUG] Compile $1 against ${fqbn}.."
  arduino-cli compile -b ${fqbn} --build-property "compiler.cpp.extra_flags=$(getBuildFlags debug)" "src/$2"
  if [[ $? -eq 0 ]]; then
    echo "[DEBUG] Uploading $1 to board on port $3.."
    arduino-cli upload -b ${fqbn} -p $3 "src/$2"
  else
    echo
    echo "Compile error, not uploading"
  fi
  ;;
"monitor")
  validatePort $2
  tools/read_serial.py $2
  ;;
*)
  syntax
  ;;
esac
