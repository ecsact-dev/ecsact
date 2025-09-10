"""
"""

load("//bazel/rules/private:ecsact_codegen.bzl", _ecsact_codegen = "ecsact_codegen")
load("//bazel/rules/private:ecsact_codegen_plugin.bzl", _ecsact_codegen_plugin = "ecsact_codegen_plugin")
load("//bazel/rules/private:ecsact_library.bzl", _ecsact_library = "ecsact_library")

ecsact_codegen = _ecsact_codegen
ecsact_codegen_plugin = _ecsact_codegen_plugin
ecsact_library = _ecsact_library
