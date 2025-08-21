EcsactCodegenPluginInfo = provider(
    doc = "",
    fields = {
        "output_extension": "Plugin name. Also used as output file extension. This must match the extension specified by the plugin.",
        "outputs": "names of the ecsact files",
        "plugin": "Path to plugin or name of builtin plugin",
        "data": "Files needed at runtime",
    },
)

def _ecsact_codegen_plugin(ctx):
    if ctx.attr.plugin and ctx.attr.plugin_path:
        fail("Cannot supply both 'plugin' and 'plugin_path' for ecsact_codegen_plugin")

    data = []
    plugin_path = ctx.attr.plugin_path
    if ctx.file.plugin:
        data.append(ctx.file.plugin)
        plugin_path = ctx.file.plugin.path

    return [
        EcsactCodegenPluginInfo(
            output_extension = ctx.attr.output_extension,
            outputs = ctx.attr.outputs,
            plugin = plugin_path,
            data = data,
        ),
    ]

ecsact_codegen_plugin = rule(
    implementation = _ecsact_codegen_plugin,
    doc = "Bazel info necessary for ecsact codegen plugin to be used with `ecsact_codegen`. Default plugins are available at `@ecsact//codegen_plugins:*`.",
    attrs = {
        "output_extension": attr.string(mandatory = False, doc = "Plugin name. Also used as output file extension. This must match the extension specified by the plugin."),
        "outputs": attr.string(mandatory = False),
        "plugin": attr.label(mandatory = False, allow_single_file = True, doc = "Label to plugin binary"),
        "plugin_path": attr.string(mandatory = False, doc = "Path to plugin or name of builtin plugin."),
    },
)
