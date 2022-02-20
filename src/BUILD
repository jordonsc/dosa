COPTS = [
    "-std=c++11",
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
        "//lib:sonar",
    ],
)

cc_binary(
    name = "monitor",
    srcs = glob(["monitor/monitor.cc"]),
    copts = ["-std=c++11"],
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//lib:monitor",
    ],
)
