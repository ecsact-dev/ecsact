def _cc_stamp_header(ctx):
    out = ctx.outputs.out
    args = ctx.actions.args()

    args.add("--stable_status", ctx.info_file)
    args.add("--volatile_status", ctx.version_file)
    args.add("--output_header", out)

    ctx.actions.run(
        outputs = [out],
        inputs = [ctx.info_file, ctx.version_file],
        arguments = [args],
        executable = ctx.executable.tool,
    )

    return DefaultInfo(files = depset([out]))

cc_stamp_header = rule(
    implementation = _cc_stamp_header,
    attrs = {
        "out": attr.output(
            doc = "C++ header file to generate",
            mandatory = True,
        ),
        "tool": attr.label(
            default = Label("//bazel/tools:cc_stamp_header_generator"),
            cfg = "exec",
            executable = True,
        ),
    },
)
