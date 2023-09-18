import platform

thresholds = {
    "pv": {
        "high": 0.75,
        "med": 0.3,
        "low": 0.05,
    },
    "battery": {
        "high": 100,
        "med": 50,
        "low": 25,
    },
    "mains": {
        0: {
            # "Summer" mode
            "activate": 40,
            "activate_time": 5,
            "deactivate": 80,
            "deactivate_time": 5,
        },
        1: {
            # "Spring" mode
            "activate": 55,
            "activate_time": 5,
            "deactivate": 85,
            "deactivate_time": 5,
        },
        2: {
            # "Winter" mode
            "activate": 70,
            "activate_time": 5,
            "deactivate": 90,
            "deactivate_time": 5,
        },
        3: {
            # "Store" mode
            "activate": 90,
            "activate_time": 5,
            "deactivate": 100,
            "deactivate_time": 5,
        }
    }
}


def is_production_machine():
    return platform.machine() == "armv7l"


def get_data_file():
    return "/etc/power_grid_data.json" if is_production_machine() else "/tmp/power_grid_data.json"


def get_config_file():
    return "/etc/power_grid_cfg.json" if is_production_machine() else "/tmp/power_grid_cfg.json"
