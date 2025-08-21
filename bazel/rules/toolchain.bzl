"""This module implements the language-specific toolchain rule.
"""

EcsactInfo = provider(
    doc = "Information about how to invoke the tool executable.",
    fields = {
        "target_tool": "",
        "target_tool_path": "Path to the tool executable for the target platform.",
        "tool_files": """Files required in runfiles to make the tool executable available.

May be empty if the target_tool_path points to a locally installed tool binary.""",
    },
)

def _ecsact_toolchain_impl(ctx):
    if ctx.attr.target_tool and ctx.attr.target_tool_path:
        fail("Can only set one of target_tool or target_tool_path but both were set.")
    if not ctx.attr.target_tool and not ctx.attr.target_tool_path:
        fail("Must set one of target_tool or target_tool_path.")

    tool_files = []
    target_tool_path = ctx.attr.target_tool_path

    if ctx.attr.target_tool:
        tool_files = ctx.attr.target_tool[DefaultInfo].files.to_list()
        tool_files.extend(ctx.attr.target_tool[DefaultInfo].default_runfiles.files.to_list())
        target_tool_path = tool_files[0].path

    # Make the $(ECSACT_BIN) variable available in places like genrules.
    # See https://docs.bazel.build/versions/main/be/make-variables.html#custom_variables
    template_variables = platform_common.TemplateVariableInfo({
        "ECSACT_BIN": target_tool_path,
    })
    default = DefaultInfo(
        files = depset(tool_files),
        runfiles = ctx.runfiles(files = tool_files),
    )

    target_tool = ctx.attr.target_tool[DefaultInfo].files_to_run if ctx.attr.target_tool else None

    ecsact_info = EcsactInfo(
        target_tool = target_tool,
        target_tool_path = target_tool_path,
        tool_files = tool_files,
    )

    toolchain_info = platform_common.ToolchainInfo(
        ecsact_info = ecsact_info,
        template_variables = template_variables,
        default = default,
    )
    return [
        default,
        toolchain_info,
        template_variables,
    ]

ecsact_toolchain = rule(
    implementation = _ecsact_toolchain_impl,
    attrs = {
        "target_tool": attr.label(
            doc = "A hermetically downloaded executable target for the target platform.",
            mandatory = False,
            allow_single_file = True,
            cfg = "exec",
        ),
        "target_tool_path": attr.string(
            doc = "Path to an existing executable for the target platform.",
            mandatory = False,
        ),
    },
    doc = """Defines a ecsact compiler/runtime toolchain.

For usage see https://docs.bazel.build/versions/main/toolchains.html#defining-toolchains.
""",
)
