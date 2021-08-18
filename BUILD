COPTS = [
    "-std=c++17",
    "-Ilib",
]
LINKOPTS = [
]

cc_library(
    name = "dosa_lib",
    srcs = glob(["src/lib/**/*.cc"]),
    hdrs = glob(["src/lib/**/*.h", "src/lib/**/*.tcc"]),
    includes = ["src/lib"],
    copts = COPTS,
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
)
