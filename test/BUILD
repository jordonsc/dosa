load("//bazel:build.bzl", "COPTS", "LINKOPTS")

# The messages library very intentionally doesn't touch any Arduino library code so that it can be easily tested.
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

# Need to get the entire Arduino library compiling with Bazel for this to work -
#cc_test(
#    name = "common",
#    size = "small",
#    srcs = [
#        "common/linked_list.cc",
#        "test.cc",
#    ],
#    copts = COPTS,
#    linkopts = LINKOPTS,
#    deps = [
#        "//lib:common",
#        "@gtest",
#    ],
#)
