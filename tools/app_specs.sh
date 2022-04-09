# Update this list to match board to application -
function getFqbn() {
  case "$1" in
  "door")
    echo "arduino:samd:nano_33_iot"
    ;;
  "relay")
    echo "arduino:samd:nano_33_iot"
    ;;
  "alarm")
    echo "arduino:samd:nano_33_iot"
    ;;
  "pir")
    echo "arduino:samd:nano_33_iot"
    ;;
  "sonar")
    echo "arduino:samd:nano_33_iot"
    ;;
  "laser")
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
  "alarm")
    echo "DOSA-A"
    ;;
  "pir")
    echo "DOSA-M"
    ;;
  "sonar")
    echo "DOSA-S"
    ;;
  "laser")
    echo "DOSA-L"
    ;;
  *) ;;

  esac
}
