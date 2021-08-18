cc_library(
    name = "arduino",
    srcs = glob([
        "arduino/**/*.cpp",
        "arduino/**/*.c",
    ]),
    hdrs = glob(["arduino/**/*.h"]),
    includes = ["arduino"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "ArduinoBLE",
    srcs = glob(["ArduinoBLE/src/**/*.cpp"]),
    hdrs = glob(["ArduinoBLE/src/**/*.h"]),
    includes = ["ArduinoBLE/src"],
    visibility = ["//visibility:public"],
)
