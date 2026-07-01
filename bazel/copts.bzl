load("@bazel_skylib//lib:selects.bzl", "selects")

# Ecsact repositories currently only support clang, cl, and emscripten
copts = selects.with_or({
    ("@rules_cc//cc/compiler:emscripten"): [
        "-std=c++23",
    ],
    ("@rules_cc//cc/compiler:clang"): [
        "-std=c++26",
    ],
    ("@rules_cc//cc/compiler:msvc-cl", "@rules_cc//cc/compiler:clang-cl"): [
        "/std:c++latest",
        "/permissive-",
        "/Zc:preprocessor",
    ],
})

linkopts = selects.with_or({
    ("@rules_cc//cc/compiler:emscripten"): [
    ],
    ("@rules_cc//cc/compiler:clang"): [
    ],
    ("@rules_cc//cc/compiler:msvc-cl", "@rules_cc//cc/compiler:clang-cl"): [
    ],
})
