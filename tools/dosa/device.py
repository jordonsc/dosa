def device_type_str(x):
    if x == 0:
        return "UNSPECIFIED"
    elif x == 10:
        return "MOTION SENSOR"
    elif x == 11:
        return "TRIP SENSOR"
    elif x == 50:
        return "SWITCH"
    elif x == 51:
        return "MOTORISED WINCH"
    else:
        return "UNKNOWN DEVICE"


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
