#include <string>
#include <filesystem>
#include "ecsact/runtime/meta.hh"
#include "ecsact/codegen/plugin.h"
#include "ecsact/codegen/plugin.hh"
#include "ecsact/lang-support/lang-cc.hh"

namespace fs = std::filesystem;

constexpr auto GENERATED_FILE_DISCLAIMER = R"(// GENERATED FILE - DO NOT EDIT
)";

void ecsact_codegen_plugin(
	ecsact_package_id          package_id,
	ecsact_codegen_write_fn_t  write_fn,
	ecsact_codegen_report_fn_t report_fn
) {
	using ecsact::cc_lang_support::c_identifier;
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::meta::get_all_system_like_ids;
	ecsact::codegen_plugin_context ctx{package_id, 0, write_fn, report_fn};

	ctx.writef(GENERATED_FILE_DISCLAIMER);

	fs::path package_systems_hh_path = ecsact_meta_package_file_path(package_id);
	package_systems_hh_path.replace_extension(
		package_systems_hh_path.extension().string() + ".systems.hh"
	);

	ctx.writef("#include \"{}\"\n", package_systems_hh_path.filename().string());

	for(auto sys_like_id : get_all_system_like_ids(ctx.package_id)) {
		std::string full_name =
			ecsact_meta_decl_full_name(ecsact_id_cast<ecsact_decl_id>(sys_like_id));

		if(full_name.empty()) {
			continue;
		}

		auto cpp_full_name = cpp_identifier(full_name);

		ctx.writef(
			"void {} (struct ecsact_system_execution_context* cctx) {{\n",
			c_identifier(full_name)
		);

		ctx.writef("\t{}::context ctx{{cctx}};\n", cpp_full_name);
		ctx.writef("\t{}::impl(ctx);\n", cpp_full_name);
		ctx.writef("}}\n");
	}
}
