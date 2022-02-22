def device_type_str(x):
    if x == 1:
        return "Monitor"
    elif x == 10:
        return "PIR Sensor"
    elif x == 11:
        return "Sonar Sensor"
    elif x == 12:
        return "Active IR Sensor"
    elif x == 13:
        return "Optical Sensor"
    elif x == 30:
        return "Button"
    elif x == 50:
        return "Switch"
    elif x == 51:
        return "Motorised Winch"
    else:
        return "Unknown Device"


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
