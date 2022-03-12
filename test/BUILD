load("//bazel:build.bzl", "COPTS", "LINKOPTS")

cc_test(
    name = "messages",
    size = "small",
    srcs = [
        "messages/ack.cc",
        "messages/log_msg.cc",
        "messages/trigger.cc",
        "test.cc",
    ],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [
        "//lib:messages",
        "@gtest",
    ],
)
