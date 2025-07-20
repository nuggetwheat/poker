# BUILD file for the C++ project.

# Load the cc_binary rule from Bazel's built-in C++ rules.
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

cc_library(
    name = "poker",
    hdrs = [
        "poker.h",
    	"holdem.h",
    ],
)

cc_binary(
    name = "poker_simulation",
    srcs = ["poker_simulation.cc"],
    deps = [":poker"]
    # Optionally, you can add compiler flags here if needed.
    # For example: copts = ["-std=c++17"],
)
