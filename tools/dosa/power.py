import platform

thresholds = {
    "pv": {
        "high": 0.75,
        "med": 0.3,
        "low": 0.05,
    },
    "battery": {
        "high": 100,
        "med": 66,
        "low": 33,
    },
    "mains": {
        0: {
            "activate": 33,
            "activate_time": 30,
            "deactivate": 66,
            "deactivate_time": 180,
        },
        1: {
            "activate": 50,
            "activate_time": 30,
            "deactivate": 75,
            "deactivate_time": 180,
        },
        2: {
            "activate": 75,
            "activate_time": 15,
            "deactivate": 100,
            "deactivate_time": 300,
        }
    }
}


def is_production_machine():
    return platform.machine() == "armv7l"


def get_data_file():
    return "/etc/power_grid_data.json" if is_production_machine() else "/tmp/power_grid_data.json"


def get_config_file():
    return "/etc/power_grid_cfg.json" if is_production_machine() else "/tmp/power_grid_cfg.json"
