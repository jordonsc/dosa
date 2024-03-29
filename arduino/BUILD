# Boards
cc_library(
    name = "board_samd",
    srcs = glob([
        "boards/samd/**/*.cpp",
        "boards/samd/**/*.c",
    ]),
    hdrs = glob(["boards/samd/**/*.h"]),
    includes = [
        "boards/samd/cores/arduino",
        "boards/samd/cores/arduino/api",
    ] + glob(
        [
            "boards/samd/libraries/*/src",
            "boards/samd/libraries/Wire",
        ],
        exclude_directories = 0,
    ),
    visibility = ["//visibility:public"],
)

cc_library(
    name = "board_megaavr",
    srcs = glob([
        "boards/megaavr/**/*.cpp",
        "boards/megaavr/**/*.c",
    ]),
    hdrs = glob(["boards/megaavr/**/*.h"]),
    includes = [
        "boards/megaavr/cores/arduino",
        "boards/megaavr/cores/arduino/api",
    ] + glob(
        [
            "boards/megaavr/libraries/*/src",
        ],
        exclude_directories = 0,
    ),
    visibility = ["//visibility:public"],
)

cc_library(
    name = "board_inkplate",
    srcs = glob([
        "boards/inkplate/**/*.cpp",
        "boards/inkplate/**/*.c",
    ]),
    hdrs = glob(["boards/inkplate/**/*.h"]),
    includes = [
        "boards/inkplate/cores/Inkplate",
    ] + glob(
        ["boards/inkplate/libraries/*/src"],
        exclude_directories = 0,
    ),
    visibility = ["//visibility:public"],
)

# Variants
cc_library(
    name = "variant_nano_33_iot",
    srcs = glob(["variants/nano_33_iot/*.cpp"]),
    hdrs = glob(["variants/nano_33_iot/*.h"]),
    includes = ["variants/nano_33_iot"],
    visibility = ["//visibility:public"],
    deps = [
    ],
)

cc_library(
    name = "variant_nona4809",
    srcs = glob(["variants/nona4809/*.cpp"]),
    hdrs = glob(["variants/nona4809/*.h"]),
    includes = ["variants/nona4809"],
    visibility = ["//visibility:public"],
    deps = [
    ],
)

cc_library(
    name = "variant_inkplate",
    srcs = glob(["variants/inkplate/*.cpp"]),
    hdrs = glob(["variants/inkplate/*.h"]),
    includes = ["variants/inkplate"],
    visibility = ["//visibility:public"],
    deps = [
    ],
)

# Libraries
cc_library(
    name = "Adafruit_BusIO",
    srcs = glob(["libraries/Adafruit_BusIO/*.cpp"]),
    hdrs = glob(["libraries/Adafruit_BusIO/*.h"]),
    includes = ["libraries/Adafruit_BusIO"],
    visibility = ["//visibility:public"],
    deps = [
    ],
)

cc_library(
    name = "Adafruit_FRAM_SPI",
    srcs = glob(["libraries/Adafruit_FRAM_SPI/*.cpp"]),
    hdrs = glob(["libraries/Adafruit_FRAM_SPI/*.h"]),
    includes = ["libraries/Adafruit_FRAM_SPI"],
    visibility = ["//visibility:public"],
    deps = [
        "//arduino:Adafruit_BusIO",
    ],
)

cc_library(
    name = "ArduinoBLE",
    srcs = glob(["libraries/ArduinoBLE/src/**/*.cpp"]),
    hdrs = glob(["libraries/ArduinoBLE/src/**/*.h"]),
    includes = ["libraries/ArduinoBLE/src"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "Array",
    srcs = glob(["libraries/Array/src/**/*.cpp"]),
    hdrs = glob(["libraries/Array/src/**/*.h"]),
    includes = ["libraries/Array/src"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "Inkplate",
    srcs = glob(["libraries/Inkplate/src/**/*.cpp"]),
    hdrs = glob(["libraries/Inkplate/src/**/*.h"]),
    includes = ["libraries/Inkplate/src"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "SparkFun_GridEYE",
    srcs = glob(["libraries/SparkFun_GridEYE_AMG88_Library/src/**/*.cpp"]),
    hdrs = glob(["libraries/SparkFun_GridEYE_AMG88_Library/src/**/*.h"]),
    includes = ["libraries/SparkFun_GridEYE_AMG88_Library/src"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "WiFiNINA",
    srcs = glob(["libraries/WiFiNINA/src/**/*.cpp"]),
    hdrs = glob(["libraries/WiFiNINA/src/**/*.h"]),
    includes = ["libraries/WiFiNINA/src"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "ArduinoOTA",
    srcs = glob(["libraries/ArduinoOTA/src/**/*.cpp"]),
    hdrs = glob(["libraries/ArduinoOTA/src/**/*.h"]),
    includes = ["libraries/ArduinoOTA/src"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "HttpClient",
    srcs = glob(["libraries/HttpClient/**/*.cpp"]),
    hdrs = glob(["libraries/HttpClient/**/*.h"]),
    includes = ["libraries/HttpClient"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "megaAVR_PWM",
    srcs = glob(["libraries/megaAVR_PWM/**/*.cpp"]),
    hdrs = glob(["libraries/megaAVR_PWM/**/*.h"]),
    includes = ["libraries/megaAVR_PWM/src"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "StatsD",
    srcs = glob(["libraries/StatsD/src/**/*.cpp"]),
    hdrs = glob(["libraries/StatsD/src/**/*.h"]),
    includes = ["libraries/StatsD/src"],
    visibility = ["//visibility:public"],
)
