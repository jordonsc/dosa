import logging
import libscrc

CHARGING_STATE = {
    0: 'deactivated',
    1: 'activated',
    2: 'mppt',
    3: 'equalizing',
    4: 'boost',
    5: 'floating',
    6: 'current limiting'
}

FUNCTION = {
    3: "READ",
    6: "WRITE"
}


def bytes_to_int(bs, offset, length):
    # Reads data from a list of bytes, and converts to an int
    ret = 0
    if len(bs) < (offset + length):
        return ret
    if length > 0:
        # offset = 11, length = 2 => 11 - 12
        byteorder = 'big'
        start = offset
        end = offset + length
    else:
        # offset = 11, length = -2 => 10 - 11
        byteorder = 'little'
        start = offset + length + 1
        end = offset + 1
    # Easier to read than the bit-shifting below
    return int.from_bytes(bs[start:end], byteorder=byteorder)


def int_to_bytes(i, pos=0):
    # Converts an integer into 2 bytes (16 bits)
    # Returns either the first or second byte as an int
    if pos == 0:
        return int(format(i, '016b')[:8], 2)
    if pos == 1:
        return int(format(i, '016b')[8:], 2)
    return 0


def create_request_payload(device_id, function, reg_addr, read_word):
    if not reg_addr:
        return None

    data = [
        device_id,
        function,
        int_to_bytes(reg_addr, 0),
        int_to_bytes(reg_addr, 1),
        int_to_bytes(read_word, 0),
        int_to_bytes(read_word, 1)
    ]

    crc = libscrc.modbus(bytes(data))
    data.append(int_to_bytes(crc, 1))
    data.append(int_to_bytes(crc, 0))
    logging.debug("Creating read request: %s => %s", reg_addr, data)

    return data


def parse_charge_controller_info(bs):
    data = {
        'function': FUNCTION[bytes_to_int(bs, 1, 1)],
        'battery_percentage': bytes_to_int(bs, 3, 2),
        'battery_voltage': bytes_to_int(bs, 5, 2) * 0.1,
        'controller_temperature': bytes_to_int(bs, 9, 1),
        'battery_temperature': bytes_to_int(bs, 10, 1),
        'load_voltage': bytes_to_int(bs, 11, 2) * 0.1,
        'load_current': bytes_to_int(bs, 13, 2) * 0.01,
        'load_power': bytes_to_int(bs, 15, 2),
        'pv_voltage': bytes_to_int(bs, 17, 2) * 0.1,
        'pv_current': bytes_to_int(bs, 19, 2) * 0.01,
        'pv_power': bytes_to_int(bs, 21, 2),
        'max_charging_power_today': bytes_to_int(bs, 33, 2),
        'max_discharging_power_today': bytes_to_int(bs, 35, 2),
        'charging_amp_hours_today': bytes_to_int(bs, 37, 2),
        'discharging_amp_hours_today': bytes_to_int(bs, 39, 2),
        'power_generation_today': bytes_to_int(bs, 41, 2),  # kWh / 10,000
        'power_generation_total': bytes_to_int(bs, 59, 4),  # kWh / 10,000
        'load_state': bytes_to_int(bs, 67, 1) & 0x80 == 0x80,
    }

    charging_status_code = bytes_to_int(bs, 67, 2) & 0x00ff
    data['charging_status'] = CHARGING_STATE[charging_status_code]

    return data


def parse_set_load_response(bs):
    data = {
        'function': FUNCTION[bytes_to_int(bs, 1, 1)],
        'load_status': bytes_to_int(bs, 5, 1)
    }

    return data
