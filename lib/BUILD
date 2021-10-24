COPTS = [
    "-std=c++17",
]

LINKOPTS = [
]

# DOSA common library
cc_library(
    name = "dosa",
    srcs = glob(["dosa/src/**/*.cc"]),
    hdrs = glob(["dosa/src/**/*.h"]),
    copts = COPTS,
    includes = ["dosa/src"],
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//arduino",
        "//arduino:ArduinoBLE",
    ],
)

# Door winch master unit library
cc_library(
    name = "door",
    srcs = glob(["door/src/**/*.cc"]),
    hdrs = glob(["door/src/**/*.h"]),
    copts = COPTS,
    includes = ["door/src"],
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//arduino",
        "//arduino:ArduinoBLE",
        "//arduino:Base64",
        "//arduino:WiFiNINA",
        "//lib:dosa",
    ],
)

# Sensor satellite unit library
cc_library(
    name = "sensor",
    srcs = glob(["sensor/src/**/*.cc"]),
    hdrs = glob(["sensor/src/**/*.h"]),
    copts = COPTS,
    includes = ["sensor/src"],
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//arduino",
        "//arduino:ArduinoBLE",
        "//lib:dosa",
    ],
)
