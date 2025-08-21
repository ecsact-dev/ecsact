"""
"""

load(":ecsact_codegen_plugin.bzl", "EcsactCodegenPluginInfo")

def _ecsact_codegen(ctx):
    info = ctx.toolchains["//ecsact:toolchain_type"].ecsact_info

    outputs = []
    tools = [] + info.tool_files

    args = ctx.actions.args()
    args.add("codegen")
    args.add_all(ctx.files.srcs)

    plugin_data = []

    for plugin in ctx.attr.plugins:
        plugin_info = plugin[EcsactCodegenPluginInfo]
        args.add("--plugin", plugin_info.plugin)
        plugin_data.extend(plugin_info.data)
        if len(plugin_info.outputs) == 0:
            for src in ctx.files.srcs:
                out_basename = src.basename + "." + plugin_info.output_extension
                out_file = ctx.attr.output_directory + "/" + out_basename
                outputs.append(ctx.actions.declare_file(out_file))
        else:
            for output in plugin_info.outputs:
                out_file = ctx.attr.output_directory + "/" + output
                outputs.append(ctx.actions.declare_file(out_file))

    args.add("--outdir", outputs[0].dirname)

    ctx.actions.run(
        mnemonic = "EcsactCodegen",
        outputs = outputs,
        inputs = ctx.files.srcs + plugin_data,
        executable = info.target_tool_path,
        tools = tools,
        arguments = [args],
    )

    return [
        DefaultInfo(
            files = depset(outputs),
        ),
    ]

ecsact_codegen = rule(
    implementation = _ecsact_codegen,
    doc = "Ecsact codegen rule. Executes `ecsact codegen` with specified plugins.",
    attrs = {
        "srcs": attr.label_list(
            allow_files = True,
            mandatory = True,
            doc = ".ecsact source files",
        ),
        "output_directory": attr.string(
            mandatory = True,
            doc = "Output directory: equivalent to the ecsact codegen --outdir flag",
        ),
        "plugins": attr.label_list(
            providers = [EcsactCodegenPluginInfo],
            mandatory = True,
            cfg = "exec",
            doc = "List of plugin to use for code generation. Default ones are available at `@ecsact//codegen_plugins:*`",
        ),
    },
    toolchains = ["//ecsact:toolchain_type"],
)
