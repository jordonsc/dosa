COPTS = [
    "-std=c++17",
]

LINKOPTS = [
]

cc_binary(
    name = "door",
    srcs = glob(["door/door.cc"]),
    copts = COPTS,
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//arduino",
        "//arduino:ArduinoBLE",
        "//lib:door",
    ],
)

cc_binary(
    name = "sensor",
    srcs = glob(["sensor/sensor.cc"]),
    copts = COPTS,
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//arduino",
        "//arduino:ArduinoBLE",
        "//lib:sensor",
    ],
)

cc_binary(
    name = "sonar",
    srcs = glob(["sonar/sonar.cc"]),
    copts = COPTS,
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//arduino",
        "//arduino:ArduinoBLE",
        "//lib:sonar",
    ],
)

cc_binary(
    name = "monitor",
    srcs = glob(["monitor/monitor.cc"]),
    copts = COPTS,
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//arduino",
    ],
)
