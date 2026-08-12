load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_library(
    name = "spsc_queue",
    hdrs = ["include/spsc_queue.hpp"],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
    copts = ["-O3", "-std=c++17"],
)

cc_binary(
    name = "spsc_benchmark",
    srcs = ["main.cpp"],
    deps = [":spsc_queue"],
    copts = ["-O3", "-std=c++17"],
    linkopts = ["-pthread"],
)
