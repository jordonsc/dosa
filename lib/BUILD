COPTS = [
    "-std=c++11",
]

LINKOPTS = [
]

# DOSA samd-based apps
cc_library(
    name = "dosa",
    srcs = glob(["dosa/src/**/*.cpp"]),
    hdrs = glob(["dosa/src/**/*.h"]),
    copts = COPTS,
    includes = ["dosa/src"],
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//arduino:Adafruit_BusIO",
        "//arduino:Adafruit_FRAM_SPI",
        "//arduino:ArduinoBLE",
        "//arduino:SparkFun_GridEYE",
        "//arduino:WiFiNINA",
        "//arduino:board_samd",
        "//lib:messages",
    ],
)

# DOSA Inkplate-based apps
cc_library(
    name = "dosa_inkplate",
    srcs = glob(["dosa_inkplate/src/**/*.cpp"]),
    hdrs = glob(["dosa_inkplate/src/**/*.h"]),
    copts = ["-std=c++11"],
    includes = ["dosa_inkplate/src"],
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//arduino:Inkplate",
        "//arduino:board_inkplate",
        "//lib:messages",
    ],
)

# Communication messages
cc_library(
    name = "messages",
    srcs = glob(["messages/src/**/*.cpp"]),
    hdrs = glob(["messages/src/**/*.h"]),
    copts = COPTS,
    includes = ["messages/src"],
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
    ],
)

# Door winch master unit library
cc_library(
    name = "door",
    srcs = glob(["door/src/**/*.cpp"]),
    hdrs = glob(["door/src/**/*.h"]),
    copts = COPTS,
    includes = ["door/src"],
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//lib:dosa",
    ],
)

# IR grid motion sensor
cc_library(
    name = "sensor",
    srcs = glob(["sensor/src/**/*.cpp"]),
    hdrs = glob(["sensor/src/**/*.h"]),
    copts = COPTS,
    includes = ["sensor/src"],
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//lib:dosa",
    ],
)

# Sonar trip sensor
cc_library(
    name = "sonar",
    srcs = glob(["sonar/src/**/*.cpp"]),
    hdrs = glob(["sonar/src/**/*.h"]),
    copts = COPTS,
    includes = ["sonar/src"],
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//lib:dosa",
    ],
)

# monitor app
cc_library(
    name = "monitor",
    srcs = glob(["monitor/src/**/*.cpp"]),
    hdrs = glob(["monitor/src/**/*.h"]),
    copts = ["-std=c++11"],
    includes = ["monitor/src"],
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//lib:dosa_inkplate",
    ],
)
