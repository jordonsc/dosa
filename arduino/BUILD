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
        "//arduino:Adafruit_BusIO",
        "//arduino:SPI",
    ],
)

cc_library(
    name = "Adafruit_BusIO",
    srcs = glob(["Adafruit_BusIO/*.cpp"]),
    hdrs = glob(["Adafruit_BusIO/*.h"]),
    includes = ["Adafruit_BusIO"],
    visibility = ["//visibility:public"],
    deps = [
        "//arduino",
    ],
)

cc_library(
    name = "SPI",
    srcs = glob(["SPI/*.cpp"]),
    hdrs = glob(["SPI/*.h"]),
    includes = ["SPI"],
    visibility = ["//visibility:public"],
    deps = [
        "//arduino",
    ],
)

cc_library(
    name = "Wire",
    srcs = glob(["Wire/*.cpp"]),
    hdrs = glob(["Wire/*.h"]),
    includes = ["Wire"],
    visibility = ["//visibility:public"],
    deps = [
        "//arduino",
    ],
)

cc_library(
    name = "SparkFun_GridEYE",
    srcs = glob(["SparkFun_GridEYE_AMG88_Library/src/**/*.cpp"]),
    hdrs = glob(["SparkFun_GridEYE_AMG88_Library/src/**/*.h"]),
    includes = ["SparkFun_GridEYE_AMG88_Library/src"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "Inkplate",
    srcs = glob(["Inkplate/src/**/*.cpp"]),
    hdrs = glob(["Inkplate/src/**/*.h"]),
    includes = ["Inkplate/src"],
    visibility = ["//visibility:public"],
)
