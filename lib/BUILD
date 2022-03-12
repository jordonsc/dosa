COPTS = [
    "-std=c++11",
]

LINKOPTS = [
]

# Generic interfaces
cc_library(
    name = "common",
    srcs = glob(["common/src/**/*.cpp"]),
    hdrs = glob(["common/src/**/*.h"]),
    copts = COPTS,
    includes = ["common/src"],
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//lib:comms",
    ],
)

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
        "//arduino:variant_nano_33_iot",
        "//lib:common",
    ],
)

# DOSA Inkplate-based apps
cc_library(
    name = "dosa_inkplate",
    srcs = glob(["dosa_inkplate/src/**/*.cpp"]),
    hdrs = glob(["dosa_inkplate/src/**/*.h"]),
    copts = COPTS,
    includes = ["dosa_inkplate/src"],
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//arduino:Inkplate",
        "//arduino:board_inkplate",
        "//arduino:variant_inkplate",
        "//lib:common",
    ],
)

# DOSA communications
cc_library(
    name = "comms",
    srcs = glob(["comms/src/**/*.cpp"]),
    hdrs = glob(["comms/src/**/*.h"]),
    copts = COPTS,
    includes = ["comms/src"],
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        # The board-specific dependencies are all a (potential) lie, and is just so your IDE can make sense of what is
        # happening
        "//arduino:WiFiNINA",
        "//arduino:board_samd",
        "//arduino:variant_nano_33_iot",
        # Requirements below are all genuine
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

# OTA support, extends normal DOSA App
cc_library(
    name = "ota",
    srcs = glob(["ota/src/**/*.cpp"]),
    hdrs = glob(["ota/src/**/*.h"]),
    copts = COPTS,
    includes = ["ota/src"],
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        # Most common hardware use-case
        "//arduino:WiFiNINA",
        "//arduino:board_samd",
        "//arduino:variant_nano_33_iot",
        # Actual deps
        "//arduino:ArduinoOTA",
        "//lib:dosa",
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
        "//lib:ota",
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
        "//lib:ota",
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
        "//lib:ota",
    ],
)

# Display monitor
cc_library(
    name = "monitor",
    srcs = glob(["monitor/src/**/*.cpp"]),
    hdrs = glob(["monitor/src/**/*.h"]),
    copts = COPTS,
    includes = ["monitor/src"],
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//arduino:Array",
        "//lib:dosa_inkplate",
    ],
)
