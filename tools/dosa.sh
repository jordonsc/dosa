#!/usr/bin/env bash

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
  echo "  dosa (compile|upload|install) (door|sensor) [port]"
  echo
  echo "compile  :  compiles the application, does not upload"
  echo "upload   :  uploads the application, does not compile"
  echo "install  :  compiles application, uploads if compile is successful"
  echo
  echo "Connected boards:"
  echo
  arduino-cli board list
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

fqbn=$(getFqbn $2)
if [ -z "$fqbn" ]; then
  echo "Invalid application, aborting"
  exit 2
fi

case $1 in
"compile")
  echo "Compile '$2' against ${fqbn}.."
  arduino-cli compile -b ${fqbn} "src/$2"
  ;;
"upload")
  validatePort $3
  echo "Uploading $2 to board on port $3.."
  arduino-cli upload -b ${fqbn} -p $3 "src/$2"
  ;;
"install")
  validatePort $3
  echo "Compile $1 against ${fqbn}.."
  arduino-cli compile -b ${fqbn} "src/$2"
  if [[ $? -eq 0 ]]; then
    echo "Uploading $1 to board on port $3.."
    arduino-cli upload -b ${fqbn} -p $3 "src/$2"
  else
    echo
    echo "Compile error, not uploading"
  fi
  ;;
*)
  syntax
  ;;
esac
