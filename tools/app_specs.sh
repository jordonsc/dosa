# Update this list to match board to application -
function getFqbn() {
  case "$1" in
  "door")
    echo "arduino:samd:nano_33_iot"
    ;;
  "relay")
    echo "arduino:samd:nano_33_iot"
    ;;
  "sensor")
    echo "arduino:samd:nano_33_iot"
    ;;
  "sonar")
    echo "arduino:samd:nano_33_iot"
    ;;
  "monitor")
    echo "Croduino_Boards:Inkplate:Inkplate6"
    ;;
  *) ;;

  esac
}

# Should match the "short name" in app config
function getAppKey() {
  case "$1" in
  "door")
    echo "DOSA-D"
    ;;
  "relay")
    echo "DOSA-R"
    ;;
  "sensor")
    echo "DOSA-M"
    ;;
  "sonar")
    echo "DOSA-S"
    ;;
  *) ;;

  esac
}
