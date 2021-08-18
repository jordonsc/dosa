COPTS = [
    "-std=c++17",
    "-Ilib",
]

LINKOPTS = [
]

cc_library(
    name = "dosa_lib",
    srcs = glob(["src/lib/**/*.cc"]),
    hdrs = glob(["src/lib/**/*.h"]),
    copts = COPTS,
    includes = ["src/lib"],
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//arduino",
        "//arduino:ArduinoBLE",
    ],
)

cc_binary(
    name = "main",
    srcs = glob(["src/main/main.cc"]),
    copts = COPTS,
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//:dosa_lib",
        "//arduino",
        "//arduino:ArduinoBLE",
    ],
)

cc_binary(
    name = "sensor",
    srcs = glob(["src/sensor/sensor.cc"]),
    copts = COPTS,
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//:dosa_lib",
        "//arduino",
        "//arduino:ArduinoBLE",
    ],
)
