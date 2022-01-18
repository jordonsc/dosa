load("//bazel:build.bzl", "COPTS", "LINKOPTS")

cc_test(
    name = "messages",
    size = "small",
    srcs = [
        "messages/comms.cc",
        "test.cc",
    ],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [
        "//lib:messages",
        "@gtest",
    ],
)
