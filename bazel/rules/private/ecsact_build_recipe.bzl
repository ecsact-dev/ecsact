load("@bazel_skylib//lib:paths.bzl", "paths")
load("@rules_cc//cc:find_cc_toolchain.bzl", "use_cc_toolchain")
load("//bazel/rules/private:ecsact_codegen_plugin.bzl", "EcsactCodegenPluginInfo")

EcsactBuildRecipeInfo = provider(
    doc = "",
    fields = {
        "recipe_path": "path to recipe yaml file",
        "data": "files needed to build recipe",
    },
)

CPP_HEADER_SUFFIXES = [
    ".hh",
    ".h",
    ".hpp",
    ".inl",
    ".ipp",
]

CPP_SOURCE_SUFFIXES = [
    ".c",
    ".cc",
    ".cpp",
    ".cxx",
    ".m",
    ".mm",
    ".S",
    ".s",
]

CppSourceCollectorInfo = provider(
    doc = "",
    fields = {
        "sources": "list of sources from cc libraries",
    },
)

def _is_cc_header(p):
    for cpp_header_suffix in CPP_HEADER_SUFFIXES:
        if p.endswith(cpp_header_suffix):
            return True
    return False

def _is_cc_source(p):
    for suffix in CPP_SOURCE_SUFFIXES:
        if p.endswith(suffix):
            return True
    return False

def _cpp_source_collector_aspect_impl(target, ctx):
    direct_sources = []
    if hasattr(ctx.rule.attr, "srcs"):
        for src in ctx.rule.files.srcs:
            if _is_cc_source(src.path):
                direct_sources.append(src)

    transitive_sources = []
    if hasattr(ctx.rule.attr, "deps"):
        for dep in ctx.rule.attr.deps:
            if CppSourceCollectorInfo in dep:
                transitive_sources.append(dep[CppSourceCollectorInfo].sources)

    return [
        CppSourceCollectorInfo(
            sources = depset(direct_sources, transitive = transitive_sources),
        ),
    ]

_cpp_source_collector_aspect = aspect(
    implementation = _cpp_source_collector_aspect_impl,
    attr_aspects = ["deps"],
    provides = [CppSourceCollectorInfo],
)

def _strip_external(p):
    # type: (string) -> string
    EXTERNAL_PREFIX = "external/"
    if p.startswith(EXTERNAL_PREFIX):
        slash_after_external = p.find("/", len(EXTERNAL_PREFIX))
        if slash_after_external != -1:
            return p[slash_after_external + 1:]
        else:
            return ""
    return p

def _source_outdir(src):
    # type: (File) -> string
    src_path = src.path
    if _is_cc_header(src.path):
        return "include/" + _strip_external(src.dirname)
    return _strip_external(src.dirname)

def _dedup_sources(source_paths):
    # TODO(zaucy): this is very inefficient, we should properly use depsets
    seen = {}
    deduped = []
    for src in source_paths:
        key = (src["outdir"], paths.basename(src["path"]))
        if not key in seen:
            seen[key] = True
            deduped.append(src)
    return deduped

def _ecsact_build_recipe(ctx):
    # type: (ctx) -> None

    recipe_yaml = ctx.actions.declare_file("{}.yml".format(ctx.attr.name))

    sources = []
    recipe_data = []
    defines_map = {}

    source_paths = []

    for src in ctx.files.srcs:
        source_paths.append({
            "path": src.path,
            "outdir": _source_outdir(src),
            "relative_to_cwd": True,
        })
        recipe_data.append(src)

    for fetch_src_outdir in ctx.attr.fetch_srcs:
        fetch_srcs = []
        for fetch_url in ctx.attr.fetch_srcs[fetch_src_outdir]:
            fetch_srcs.append({
                "fetch": fetch_url,
                "outdir": fetch_src_outdir,
            })
        sources.extend(fetch_srcs)

    for codegen_plugin in ctx.attr.codegen_plugins:
        info = codegen_plugin[EcsactCodegenPluginInfo]
        sources.append({
            "codegen": info.plugin.path,
            "outdir": ctx.attr.codegen_plugins[codegen_plugin],
        })
        recipe_data.append(info.plugin)

    cc_dep_compilation_contexts = []
    for cc_dep in ctx.attr.cc_deps:
        cc_info = cc_dep[CcInfo]
        cc_dep_compilation_contexts.append(cc_info.compilation_context)

        source_info = cc_dep[CppSourceCollectorInfo]
        for src in source_info.sources.to_list():
            source_paths.append({
                "path": src.path,
                "outdir": _source_outdir(src),
                "relative_to_cwd": True,
            })
            recipe_data.append(src)

    if len(cc_dep_compilation_contexts) > 0:
        cc_dep_compilation_context = cc_common.merge_compilation_contexts(compilation_contexts = cc_dep_compilation_contexts)
        defines = cc_dep_compilation_context.defines.to_list() + cc_dep_compilation_context.local_defines.to_list()

        for d in defines:
            pair = d.split("=", 1)
            key = pair[0]
            value = pair[1] if len(pair) > 1 else ""

            defines_map[key] = value

        for hdr in cc_dep_compilation_context.headers.to_list():
            hdr_prefix = ""

            if not _is_cc_header(hdr.path):
                continue

            for quote_inc in cc_dep_compilation_context.quote_includes.to_list():
                if hdr.path.startswith(quote_inc):
                    hdr_prefix = quote_inc
                    break
            for sys_inc in cc_dep_compilation_context.system_includes.to_list():
                if hdr.path.startswith(sys_inc):
                    hdr_prefix = sys_inc
                    break
            for inc in cc_dep_compilation_context.includes.to_list():
                if hdr.path.startswith(inc):
                    hdr_prefix = inc
                    break

            hdr_prefix_base = ""
            if hdr_prefix:
                hdr_prefix_base = hdr.path.removeprefix(hdr_prefix)
                hdr_prefix_base_idx = hdr_prefix_base.rindex("/")
                hdr_prefix_base = hdr_prefix_base[:hdr_prefix_base_idx]
            else:
                hdr_prefix_base_idx = hdr.path.rindex("/")
                hdr_prefix_base = "/" + hdr.path[:hdr_prefix_base_idx]
            source_paths.append({
                "path": hdr.path,
                "outdir": "include" + hdr_prefix_base,
                "relative_to_cwd": True,
            })
            recipe_data.append(hdr)

    sources.extend(_dedup_sources(source_paths))

    recipe = {
        "name": ctx.attr.name,
        "defines": defines_map,
        "sources": sources,
        "imports": ctx.attr.imports,
        "system_libs": ctx.attr.system_libs,
        "exports": ctx.attr.exports,
    }

    ctx.actions.write(recipe_yaml, json.encode(recipe))

    return [
        DefaultInfo(
            files = depset([recipe_yaml]),
        ),
        EcsactBuildRecipeInfo(
            recipe_path = recipe_yaml,
            data = recipe_data,
        ),
    ]

ecsact_build_recipe = rule(
    implementation = _ecsact_build_recipe,
    attrs = {
        "srcs": attr.label_list(
            allow_files = True,
        ),
        "defines": attr.string_dict(),
        "cc_deps": attr.label_list(
            providers = [CcInfo],
            aspects = [_cpp_source_collector_aspect],
        ),
        "fetch_srcs": attr.string_list_dict(
            allow_empty = True,
        ),
        "system_libs": attr.string_list(
            allow_empty = True,
        ),
        "codegen_plugins": attr.label_keyed_string_dict(
            providers = [EcsactCodegenPluginInfo],
        ),
        "imports": attr.string_list(),
        "exports": attr.string_list(
            mandatory = True,
        ),
    },
)

def _ecsact_build_recipe_bundle(ctx):
    # type: (ctx) -> None

    ecsact_toolchain = ctx.toolchains["@ecsact//bazel/rules:toolchain_type"].ecsact_info
    bundle_output_file = ctx.actions.declare_file("{}.ecsact-recipe-bundle".format(ctx.attr.name))

    args = ctx.actions.args()
    args.add("recipe-bundle")
    args.add_all(ctx.files.recipes)
    args.add("-o", bundle_output_file)

    report_filter = ctx.var.get("ECSACT_CLI_REPORT_FILTER", "errors_and_warnings")
    args.add("--report_filter", report_filter)

    executable = ecsact_toolchain.target_tool if ecsact_toolchain.target_tool != None else ecsact_toolchain.target_tool_path

    recipes_data = []
    for recipe in ctx.attr.recipes:
        recipes_data.extend(recipe[EcsactBuildRecipeInfo].data)

    ctx.actions.run(
        mnemonic = "EcsactRecipeBundle",
        progress_message = "Bundling Ecsact Build Recipe %{output}",
        outputs = [bundle_output_file],
        inputs = ctx.files.recipes + recipes_data,
        executable = executable,
        arguments = [args],
        toolchain = Label("@ecsact//bazel/rules:toolchain_type"),
        # need curl in PATH due to https://github.com/ecsact-dev/ecsact_cli/issues/115
        use_default_shell_env = True,
    )
    return [
        DefaultInfo(
            files = depset([bundle_output_file]),
        ),
        EcsactBuildRecipeInfo(
            recipe_path = bundle_output_file,
            data = [],
        ),
    ]

ecsact_build_recipe_bundle = rule(
    implementation = _ecsact_build_recipe_bundle,
    attrs = {
        "recipes": attr.label_list(
            providers = [EcsactBuildRecipeInfo],
        ),
    },
    toolchains = ["@ecsact//bazel/rules:toolchain_type"] + use_cc_toolchain(),
)
