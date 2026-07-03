#include <set>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include "ecsact/codegen/plugin.hh"
#include "ecsact/runtime/meta.hh"
#include "ecsact/lang-support/lang-cc.hh"

auto ecsact_codegen_output_filenames(
	ecsact_package_id package_id,
	char* const*      out_filenames,
	int32_t           max_filenames,
	int32_t           max_filename_length,
	int32_t*          out_filenames_length
) -> void {
	if(ecsact::meta::main_package() == package_id) {
		auto pkg_filename =
			ecsact::meta::package_file_path(package_id).filename().string();
		ecsact::set_codegen_plugin_output_filenames(
			std::array{
				pkg_filename + ".notify.ecsact",
			},
			out_filenames,
			max_filenames,
			max_filename_length,
			out_filenames_length
		);
	}
}

static auto process_package(
	ecsact::codegen_plugin_context& ctx,
	ecsact_package_id               package_id,
	std::set<ecsact_package_id>&    processed_packages
) -> void {
	if(processed_packages.contains(package_id)) {
		return;
	}

	for(auto comp_id : ecsact::meta::get_component_ids(package_id)) {
		const auto full_name = ecsact::meta::decl_full_name(comp_id);
		const auto sys_name_base = std::format(
			"P{}C{}",
			static_cast<int32_t>(package_id),
			static_cast<int32_t>(comp_id)
		);

		ctx.writef("system {}OnInit {{\n", sys_name_base);
		ctx.writef("\treadonly {};\n", full_name);
		ctx.writef("\tnotify oninit;\n");
		ctx.writef("}}\n\n");

		ctx.writef("system {}OnChange {{\n", sys_name_base);
		ctx.writef("\treadonly {};\n", full_name);
		ctx.writef("\tnotify onchange;\n");
		ctx.writef("}}\n\n");

		ctx.writef("system {}OnRemove {{\n", sys_name_base);
		ctx.writef("\treadonly {};\n", full_name);
		ctx.writef("\tnotify {{\n");
		ctx.writef("\t\tonremove {};\n", full_name);
		ctx.writef("\t}}\n");
		ctx.writef("}}\n\n");
	}

	processed_packages.insert(package_id);

	auto deps = ecsact::meta::get_dependencies(package_id);
	for(auto dep_pkg_id : deps) {
		process_package(ctx, dep_pkg_id, processed_packages);
	}
}

auto ecsact_codegen_plugin(
	ecsact_package_id          package_id,
	ecsact_codegen_write_fn_t  write_fn,
	ecsact_codegen_report_fn_t report_fn
) -> void {
	using ecsact::cc_lang_support::c_identifier;

	auto ctx = ecsact::codegen_plugin_context{package_id, 0, write_fn, report_fn};

	if(ecsact::meta::main_package() != package_id) {
		ctx.writef("// GENERATED FILE - DO NOT EDIT\n");
		ctx.writef(
			"// empty on purpose\n"
			"// ecsact_codegen_notify only generates for the main package\n\n"
		);
		return;
	}

	auto pkg_name = ecsact::meta::package_name(package_id);

	ctx.writef("// GENERATED FILE - DO NOT EDIT\n\n");
	ctx.writef("package ecsact.notify;\n\n");
	ctx.writef("import {};\n", pkg_name);

	auto deps = ecsact::meta::get_dependencies(package_id);
	for(auto dep_pkg_id : deps) {
		auto dep_pkg_name = ecsact::meta::package_name(dep_pkg_id);
		ctx.writef("import {};\n", dep_pkg_name);
	}
	ctx.writef("\n");

	auto processed_packages = std::set<ecsact_package_id>{};
	process_package(ctx, package_id, processed_packages);
}
