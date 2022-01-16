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

cc_library(
    name = "WiFiNINA",
    srcs = glob(["WiFiNINA/src/**/*.cpp"]),
    hdrs = glob(["WiFiNINA/src/**/*.h"]),
    includes = ["WiFiNINA/src"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "Adafruit_FRAM_SPI",
    srcs = glob(["Adafruit_FRAM_SPI/*.cpp"]),
    hdrs = glob(["Adafruit_FRAM_SPI/*.h"]),
    includes = ["Adafruit_FRAM_SPI"],
    visibility = ["//visibility:public"],
    deps = [
        "//arduino",
    ],
)
