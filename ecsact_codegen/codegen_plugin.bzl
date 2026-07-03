"""
Exports `cc_ecsact_codegen_plugin`
"""

load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_test")
load("//bazel:copts.bzl", _copts = "copts")

EcsactCodegenPluginInfo = provider(
    doc = "",
    fields = {
        "requires_main_package": "Indicates the plugin requires having a main ecsact package file",
        "output_only_from_main_package": "if True there is only one output file and it should be from the main source file",
        "output_extension": "Plugin name. Also used as output file extension. This must match the extension specified by the plugin.",
        "plugin": "Path to plugin or name of builtin plugin",
        "data": "Files needed at runtime",
    },
)

def _ecsact_codegen_plugin(ctx):
    return [
        EcsactCodegenPluginInfo(
            output_extension = ctx.attr.output_extension,
            plugin = ctx.attr.plugin,
            data = [],
        ),
    ]

ecsact_codegen_plugin = rule(
    implementation = _ecsact_codegen_plugin,
    doc = "Bazel info necessary for ecsact codegen plugin to be used with `ecsact_codegen`. Default plugins are available at `@ecsact//codegen_plugins:*`.",
    attrs = {
        "requires_main_package": attr.bool(mandatory = False, default = False),
        "output_extension": attr.string(mandatory = True, doc = "Plugin name. Also used as output file extension. This must match the extension specified by the plugin."),
        "plugin": attr.string(mandatory = True, doc = "Path to plugin or name of builtin plugin."),
    },
)
