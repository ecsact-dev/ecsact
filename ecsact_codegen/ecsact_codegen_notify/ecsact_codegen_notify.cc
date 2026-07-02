#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include "ecsact/codegen/plugin.hh"
#include "ecsact/runtime/meta.hh"

auto ecsact_codegen_output_filenames(
	ecsact_package_id package_id,
	char* const*      out_filenames,
	int32_t           max_filenames,
	int32_t           max_filename_length,
	int32_t*          out_filenames_length
) -> void {
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

auto ecsact_codegen_plugin(
	ecsact_package_id          package_id,
	ecsact_codegen_write_fn_t  write_fn,
	ecsact_codegen_report_fn_t report_fn
) -> void {
	ecsact::codegen_plugin_context ctx{package_id, 0, write_fn, report_fn};

	auto pkg_name = ecsact::meta::package_name(package_id);

	ctx.writef("// GENERATED FILE - DO NOT EDIT\n\n");
	ctx.writef("package ecsact.notify;\n\n");
	ctx.writef("import {};\n\n", pkg_name);

	for(auto comp_id : ecsact::meta::get_component_ids(package_id)) {
		auto full_name = ecsact::meta::decl_full_name(comp_id);
		auto sys_name_base = full_name;
		sys_name_base.erase(
			std::remove_if(
				sys_name_base.begin(),
				sys_name_base.end(),
				[](char c) { return c == '.' || c == '_'; }
			),
			sys_name_base.end()
		);

		// system <sys_name_base>OnInit
		ctx.writef("system {}OnInit {{\n", sys_name_base);
		ctx.writef("\treadonly {};\n", full_name);
		ctx.writef("\tnotify oninit;\n");
		ctx.writef("}}\n\n");

		// system <sys_name_base>OnChange
		ctx.writef("system {}OnChange {{\n", sys_name_base);
		ctx.writef("\treadonly {};\n", full_name);
		ctx.writef("\tnotify onchange;\n");
		ctx.writef("}}\n\n");

		// system <sys_name_base>OnRemove
		ctx.writef("system {}OnRemove {{\n", sys_name_base);
		ctx.writef("\treadonly {};\n", full_name);
		ctx.writef("\tnotify {{\n");
		ctx.writef("\t\tonremove {};\n", full_name);
		ctx.writef("\t}}\n");
		ctx.writef("}}\n\n");
	}
}
