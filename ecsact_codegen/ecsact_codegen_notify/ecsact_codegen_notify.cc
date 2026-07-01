#include <string>
#include <vector>
#include <array>
#include "ecsact/codegen/plugin.hh"
#include "ecsact/runtime/meta.hh"

auto ecsact_codegen_output_filenames(
	ecsact_package_id package_id,
	char* const*      out_filenames,
	int32_t           max_filenames,
	int32_t           max_filename_length,
	int32_t*          out_filenames_length
) -> void {
	auto pkg_basename = ecsact::meta::package_file_path(package_id)
												.filename()
												.replace_extension("")
												.string();
	ecsact::set_codegen_plugin_output_filenames(
		std::array{
			pkg_basename + ".notify.ecsact",
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
	ctx.writef("package {};\n\n", pkg_name);

	for(auto comp_id : ecsact::meta::get_component_ids(package_id)) {
		auto comp_name = ecsact::meta::component_name(comp_id);

		// system <component_name>_OnInit
		ctx.writef("system {}_OnInit {{\n", comp_name);
		ctx.writef("\treadonly {};\n", comp_name);
		ctx.writef("\tnotify oninit;\n");
		ctx.writef("}}\n\n");

		// system <component_name>_OnChange
		ctx.writef("system {}_OnChange {{\n", comp_name);
		ctx.writef("\treadonly {};\n", comp_name);
		ctx.writef("\tnotify onchange;\n");
		ctx.writef("}}\n\n");

		// system <component_name>_OnRemove
		ctx.writef("system {}_OnRemove {{\n", comp_name);
		ctx.writef("\treadonly {};\n", comp_name);
		ctx.writef("\tnotify onremove;\n");
		ctx.writef("}}\n\n");
	}
}
