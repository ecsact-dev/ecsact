load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

_EXPORTS_ALL_BUILD_FILE_CONTENTS = """
package(default_visibility = ["//visibility:public"])
exports_files(glob["**/*"])
"""

_ECSACT_TOOLCHAIN_SDK = """
# Ecsact toolchain from your locally installed SDK
# https://github.com/ecsact-dev/ecsact_sdk
ecsact_toolchain(name = "ecsact_sdk_system", target_tool_path = "{ecsact_exe_path}")
toolchain(name = "ecsact_sdk_system_toolchain", toolchain = ":ecsact_sdk_system", toolchain_type = "//bazel/rules:toolchain_type")
"""

_ECSACT_TOOLCHAIN_CLI = """
# Ecsact toolchain from the ecsact_cli bazel module
# https://github.com/ecsact-dev/ecsact_cli
ecsact_toolchain(name = "ecsact_cli", target_tool = "@ecsact_cli")
toolchain(name = "ecsact_cli_toolchain", toolchain = ":ecsact_cli", toolchain_type = "//bazel/rules:toolchain_type")
"""

def _ecsact_toolchain_repository_impl(rctx):
    # type: (repository_ctx) -> None

    build_file_contents = 'load("//bazel/rules:toolchain.bzl", "ecsact_toolchain")\n'
    build_file_contents += 'package(default_visibility = ["//visibility:public"])\n\n'

    if rctx.attr.ecsact_system_sdk_exe:
        build_file_contents += _ECSACT_TOOLCHAIN_SDK.format(
            ecsact_exe_path = rctx.attr.ecsact_system_sdk_exe.replace("\\", "/"),
        )

    if rctx.attr.use_ecsact_cli:
        build_file_contents += _ECSACT_TOOLCHAIN_CLI

    rctx.file(
        "BUILD.bazel",
        executable = False,
        content = build_file_contents,
    )

_ecsact_toolchain_repository = repository_rule(
    implementation = _ecsact_toolchain_repository_impl,
    attrs = {
        "ecsact_system_sdk_exe": attr.string(mandatory = False),
        "use_ecsact_cli": attr.bool(mandatory = True),
    },
)

def _windows_find_ecsact_from_app_installer(mctx):
    # type: (module_ctx) -> path

    # ctx.which doesn't work for everything available on Windows. Using cmd's
    # built-in 'where' command can find programs installed from the Microsoft
    # store or MSIX packages.
    cmd = mctx.which("cmd.exe")
    if cmd:
        where_result = mctx.execute([cmd, "/C", "where ecsact.exe"])
        if where_result.stdout:
            return mctx.path(where_result.stdout.strip())

    return None

def _get_escact_system_sdk(mctx, wanted_ecsact_version = None):
    # type: (module_ctx, string) -> string

    ecsact_exe = mctx.which("ecsact")

    if ecsact_exe == None and mctx.os.name.startswith("windows"):
        ecsact_exe = _windows_find_ecsact_from_app_installer(mctx)

    if ecsact_exe == None:
        return None

    ecsact_version_output = mctx.execute([ecsact_exe, "--version"])
    if ecsact_version_output.return_code != 0:
        fail(
            "Failed to get Ecsact SDK version (exit code {}). " +
            "Please make sure it's installed correctly.\n" +
            "\tEcsact Executable = {}\n".format(ecsact_exe) +
            "\tExit Code = {}\n".format(ecsact_version_output.return_code),
        )

    found_ecsact_version = ecsact_version_output.stdout.strip()
    if wanted_ecsact_version != None:
        # workaround for https://github.com/ecsact-dev/ecsact_sdk/issues/293
        if found_ecsact_version != wanted_ecsact_version and found_ecsact_version != "refs/tags/{}".format(wanted_ecsact_version):
            fail("Wanted Ecsact SDK {}, but {} is installed on your system".format(wanted_ecsact_version, found_ecsact_version))

    return ecsact_exe

def _ecsact_impl(mctx):
    # type: (module_ctx) -> None

    wanted_ecsact_version = None
    use_ecsact_cli = False

    for mod in mctx.modules:
        for toolchain in mod.tags.toolchain:
            if toolchain.use_ecsact_cli:
                use_ecsact_cli = True

            if toolchain.version:
                if wanted_ecsact_version != None:
                    fail("ecsact extension toolchain called multiple times. Please only call it once.")
                wanted_ecsact_version = toolchain.version

    ecsact_exe = _get_escact_system_sdk(mctx, wanted_ecsact_version)

    if ecsact_exe != None:
        ecsact_exe = str(ecsact_exe)

    _ecsact_toolchain_repository(
        name = "ecsact_toolchain",
        ecsact_system_sdk_exe = ecsact_exe,
        use_ecsact_cli = use_ecsact_cli,
    )

ecsact = module_extension(
    implementation = _ecsact_impl,
    tag_classes = {
        "toolchain": tag_class(attrs = {
            "use_ecsact_cli": attr.bool(mandatory = False, default = False),
            "version": attr.string(mandatory = False),
        }),
    },
)
