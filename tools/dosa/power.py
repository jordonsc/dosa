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
            "activate": 50,
            "activate_time": 15,
            "deactivate": 80,
            "deactivate_time": 60,
        },
        1: {
            # "Winter" mode
            "activate": 75,
            "activate_time": 15,
            "deactivate": 95,
            "deactivate_time": 60,
        },
        2: {
            # "Store" mode
            "activate": 90,
            "activate_time": 15,
            "deactivate": 100,
            "deactivate_time": 180,
        }
    }
}


def is_production_machine():
    return platform.machine() == "armv7l"


def get_data_file():
    return "/etc/power_grid_data.json" if is_production_machine() else "/tmp/power_grid_data.json"


def get_config_file():
    return "/etc/power_grid_cfg.json" if is_production_machine() else "/tmp/power_grid_cfg.json"
