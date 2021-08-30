COPTS = [
    "-std=c++17",
    "-Ilib",
]

LINKOPTS = [
]

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
        "//lib:dosa",
    ],
)
