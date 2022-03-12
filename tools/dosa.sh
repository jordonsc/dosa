#!/usr/bin/env bash

app=$(python -c "import os; print(os.path.dirname(os.path.realpath(\"$0\")))")
cd ${app}/..

ota_bucket="dosa-ota"
dosa_version=${DOSA_VERSION}
if [[ -z "${dosa_version}" ]]; then
  dosa_version=$(cat lib/common/src/const.h | grep "#define DOSA_VERSION" | awk '{print $3}')
fi

source tools/app_specs.sh

function syntax() {
  echo "DOSA v${dosa_version}"
  echo
  echo "Usage: "
  echo "  dosa (COMMAND) [APPLICATION] [PORT]"
  echo
  echo "Commands:"
  echo "  setup          :  Setup or update your local environment dependencies"
  echo "  compile        :  Compiles the application, does not upload"
  echo "  compile-debug  :  Compiles the application in debug mode, does not upload"
  echo "  upload         :  Uploads the last compiled application"
  echo "  install        :  Compiles application, uploads if compile is successful"
  echo "  install-debug  :  Compiles application in debug mode, uploads if compile is successful"
  echo "  debug          :  Runs install-debug followed by monitor for the same device"
  echo "  ota            :  Update OTA repository for given application"
  echo "  monitor        :  Opens a serial monitor to provided port"
  echo
  echo "Applications:"
  echo "  door           : Winch door driver"
  echo "  sensor         : Passive infrared motion sensor"
  echo "  sonar          : Sonar ranging trigger sensor"
  echo "  monitor        : DOSA display monitor"
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

if [[ "$1" != "setup" ]]; then
  if [[ $# -lt 2 ]]; then
    syntax
  fi
fi

function validateApp() {
  if [ -z "$1" ]; then
    echo "Invalid application, aborting"
    exit 2
  fi
}

function getBuildFlags() {
  if [[ "$1" == "debug" ]]; then
    echo -n "-DDOSA_DEBUG=1 "
  fi

  if [[ -n "${DOSA_VERSION}" ]]; then
    echo -n "-DDOSA_VERSION=${DOSA_VERSION} "
  fi
}

fqbn=$(getFqbn $2)

case $1 in
"setup")
  tools/setup.sh && tools/link_libs.sh
  exit $?
  ;;
"compile")
  validateApp $fqbn
  echo "Compile '$2' against ${fqbn}.."
  arduino-cli compile -b ${fqbn} --build-property "compiler.cpp.extra_flags=$(getBuildFlags)" "src/$2"
  exit $?
  ;;
"compile-debug")
  validateApp $fqbn
  echo "[DEBUG] Compile '$2' against ${fqbn}.."
  arduino-cli compile -b ${fqbn} --build-property "compiler.cpp.extra_flags=$(getBuildFlags debug)" "src/$2"
  exit $?
  ;;
"upload")
  validateApp $fqbn
  validatePort $3
  echo "Uploading $2 to board on port $3.."
  arduino-cli upload -b ${fqbn} -p $3 "src/$2"
  exit $?
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
    exit 1
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
    exit 1
  fi
  ;;
"debug")
  validateApp $fqbn
  validatePort $3
  echo "[DEBUG] Compile $1 against ${fqbn}.."
  arduino-cli compile -b ${fqbn} --build-property "compiler.cpp.extra_flags=$(getBuildFlags debug)" "src/$2"
  if [[ $? -eq 0 ]]; then
    echo "[DEBUG] Uploading $1 to board on port $3.."
    arduino-cli upload -b ${fqbn} -p $3 "src/$2"
    sleep 2
    echo
    tools/read_serial.py $3
  else
    echo
    echo "Compile error, not uploading"
    exit 1
  fi
  ;;
"ota")
  app_key="$(getAppKey $2)"
  if [[ -z "${app_key}" ]]; then
    echo
    echo "Missing app key for $2! (not an OTA application?)"
    exit 1
  fi

  validateApp $fqbn

  echo "Deploy OTA for ${app_key} version ${dosa_version}.."

  echo -n "Validating bucket access.. "
  gsutil ls 2>/dev/null | grep "gs://${ota_bucket}/" &>/dev/null
  if [[ $? -eq 0 ]]; then
    echo "OK"
  else
    echo "no access"
    echo
    echo "Authenticate with:"
    echo "  gcloud auth login"
    echo
    exit 1
  fi

  echo
  echo "Compile '$2' against ${fqbn}.."
  arduino-cli compile -eb ${fqbn} --build-property "compiler.cpp.extra_flags=$(getBuildFlags)" "src/$2"

  if [[ $? -eq 0 ]]; then
    echo "Uploading to GCP.."
    echo ${dosa_version} >/tmp/dosa.version
    gsutil cp /tmp/dosa.version gs://${ota_bucket}/${app_key}/version
    rm /tmp/dosa.version
    gsutil cp "src/$2/build/${fqbn//:/.}/$2.ino.bin" gs://${ota_bucket}/${app_key}/build-${dosa_version}.bin
    rm -rf "src/$2/build"

    echo
    echo "Deployment complete."
  else
    echo
    echo "Compile error, aborting"
    exit 1
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
