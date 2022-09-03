load("//bazel:build.bzl", "COPTS", "LINKOPTS")

cc_test(
    name = "messages",
    size = "small",
    srcs = [
        "messages/ack.cc",
        "messages/alt.cc",
        "messages/log_msg.cc",
        "messages/trigger.cc",
        "messages/stat.cc",
        "test.cc",
    ],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [
        "//lib:messages",
        "@gtest",
    ],
)
