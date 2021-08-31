COPTS = [
    "-std=c++17",
    "-Ilib",
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
