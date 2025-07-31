# BUILD file for the C++ project.

# Load the cc_binary rule from Bazel's built-in C++ rules.
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

cc_proto_library(
    name = "cards_cc_proto",
    deps = [":cards_proto"],
)

proto_library(
    name = "cards_proto",
    srcs = [ "cards.proto" ],
)

cc_proto_library(
    name = "poker_cc_proto",
    deps = [":poker_proto"],
)

proto_library(
    name = "poker_proto",
    srcs = [ "poker.proto" ],
    deps = [ ":cards_proto" ],
)

cc_library(
    name = "poker",
    hdrs = [
        "cards.h",
        "holdem.h",
        "player_model.h",
        "player_model_holdem.h",
        "poker.h",
    ],
    srcs = [
        "cards.cc",
        "holdem.cc",
        "player_model_holdem.cc",
        "poker.cc",
    ],
    deps = [
        ":cards_cc_proto",
        ":poker_cc_proto",
    ],
)

cc_library(
    name = "statistics",
    hdrs = [
        "holdem_stats.h",
        "poker_simulation_args.h",
    ],
    srcs = [
        "holdem_stats.cc",
    ],
    deps = [
        ":poker",
        ":cards_cc_proto",
        ":poker_cc_proto",
    ],
)

cc_binary(
    name = "poker_simulation",
    srcs = [
        "poker_simulation.cc",
        "poker_simulation_args.cc",
        "poker_simulation_args.h",
        "poker_simulation_utils.h",
    ],
    deps = [
        ":cards_cc_proto",
        ":poker_cc_proto",
        ":poker",
        ":statistics",
        "@com_google_protobuf//:protobuf",
    ],
    copts = ["-std=c++17"]
)

cc_test(
    name = "holdem_test",
    size = "small",
    srcs = ["holdem_test.cc"],
    deps = [
        ":cards_cc_proto",
        ":poker",
        ":poker_cc_proto",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
    copts = ["-std=c++17"]
)

cc_test(
    name = "poker_test",
    size = "small",
    srcs = ["poker_test.cc"],
    deps = [
        ":cards_cc_proto",
        ":poker",
        ":poker_cc_proto",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
    copts = ["-std=c++17"]
)
