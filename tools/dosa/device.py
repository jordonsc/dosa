class DeviceType:
    UNKNOWN = 0
    MONITOR = 1
    UTILITY = 2
    IR_PASSIVE = 10
    IR_ACTIVE = 11
    OPTICAL = 12
    SONAR = 20
    BUTTON = 40
    TOGGLE = 41
    POWER_TOGGLE = 110
    MOTOR = 112
    LIGHT = 113


def device_type_str(x):
    if x == 1:
        return "Monitor"
    if x == 1:
        return "Utility"
    elif x == 10:
        return "PIR Sensor"
    elif x == 11:
        return "Laser Sensor"
    elif x == 12:
        return "Optical Sensor"
    elif x == 20:
        return "Sonar Sensor"
    elif x == 40:
        return "Push Button"
    elif x == 41:
        return "Toggle Switch"
    elif x == 110:
        return "Power Toggle"
    elif x == 112:
        return "Motorised Winch"
    elif x == 113:
        return "Light Controller"
    else:
        return "Unknown (" + str(x) + ")"


def device_status_str(x):
    if x == 0:
        return "OK"
    elif x == 1:
        return "ACTIVE"
    elif x == 10:
        return "MINOR FAULT"
    elif x == 11:
        return "MAJOR FAULT"
    elif x == 12:
        return "CRITICAL"
    else:
        return "UNKNOWN STATE"
