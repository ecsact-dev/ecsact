EcsactLibraryInfo = provider()

def _ecsact_library(ctx):
    return [EcsactLibraryInfo()]

ecsact_library = rule(
    implementation = _ecsact_library,
    attrs = {
        "srcs": attr.label_list(
            allow_files = [".ecsact"],
        ),
        # "deps": attr.label_list(
        #     allow_rules = [EcsactLibraryInfo],
        # ),
    },
    toolchains = ["//ecsact:toolchain_type"],
)
