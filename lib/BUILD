load("//bazel:build.bzl", "COPTS", "LINKOPTS")

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
        "//arduino:StatsD",
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
        "//arduino:HttpClient",
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
        "//lib:sonar",
    ],
)

# IR grid motion sensor
cc_library(
    name = "pir",
    srcs = glob(["pir/src/**/*.cpp"]),
    hdrs = glob(["pir/src/**/*.h"]),
    copts = COPTS,
    includes = ["pir/src"],
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//lib:ota",
    ],
)

# Security alarm
cc_library(
    name = "alarm",
    srcs = glob(["alarm/src/**/*.cpp"]),
    hdrs = glob(["alarm/src/**/*.h"]),
    copts = COPTS,
    includes = ["alarm/src"],
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//lib:ota",
    ],
)

# Ranging trip sensor base application
cc_library(
    name = "ranging",
    srcs = glob(["ranging/src/**/*.cpp"]),
    hdrs = glob(["ranging/src/**/*.h"]),
    copts = COPTS,
    includes = ["ranging/src"],
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
        "//lib:ranging",
    ],
)

# Laser trip sensor
cc_library(
    name = "laser",
    srcs = glob(["laser/src/**/*.cpp"]),
    hdrs = glob(["laser/src/**/*.h"]),
    copts = COPTS,
    includes = ["laser/src"],
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//lib:ranging",
    ],
)

# Relay switch
cc_library(
    name = "relay",
    srcs = glob(["relay/src/**/*.cpp"]),
    hdrs = glob(["relay/src/**/*.h"]),
    copts = COPTS,
    includes = ["relay/src"],
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//lib:ota",
    ],
)

# PWM controller
cc_library(
    name = "pwm",
    srcs = glob(["pwm/src/**/*.cpp"]),
    hdrs = glob(["pwm/src/**/*.h"]),
    copts = COPTS,
    includes = ["pwm/src"],
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//arduino:board_megaavr",
        "//arduino:variant_nona4809",
        "//arduino:megaAVR_PWM",
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
