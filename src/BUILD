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
    name = "pir",
    srcs = glob(["pir/pir.cc"]),
    copts = COPTS,
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//lib:pir",
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
    name = "relay",
    srcs = glob(["relay/relay.cc"]),
    copts = COPTS,
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//lib:relay",
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
