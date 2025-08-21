#include <vector>
#include <string>
#include <cassert>
#include <cstring>
#include <algorithm>
#include <format>
#include "ecsact/runtime/meta.hh"
#include "ecsact/codegen/plugin.h"
#include "ecsact/codegen/plugin.hh"
#include "ecsact/lang-support/lang-cc.hh"

constexpr auto GENERATED_FILE_DISCLAIMER = R"(// GENERATED FILE - DO NOT EDIT
)";

static std::vector<ecsact_system_id> get_system_ids( //
	ecsact_package_id package_id
) {
	std::vector<ecsact_system_id> system_ids;
	system_ids.resize(ecsact_meta_count_systems(package_id));
	ecsact_meta_get_system_ids(
		package_id,
		static_cast<int32_t>(system_ids.size()),
		system_ids.data(),
		nullptr
	);

	return system_ids;
}

static std::vector<ecsact_action_id> get_action_ids( //
	ecsact_package_id package_id
) {
	std::vector<ecsact_action_id> action_ids;
	action_ids.resize(ecsact_meta_count_actions(package_id));
	ecsact_meta_get_action_ids(
		package_id,
		static_cast<int32_t>(action_ids.size()),
		action_ids.data(),
		nullptr
	);

	return action_ids;
}

static std::string make_package_inc_guard_str(ecsact_package_id package_id) {
	using ecsact::cc_lang_support::c_identifier;

	auto package_name = ecsact_meta_package_name(package_id);
	auto inc_guard_str = c_identifier(package_name);
	std::transform(
		inc_guard_str.begin(),
		inc_guard_str.end(),
		inc_guard_str.begin(),
		[](char c) { return static_cast<char>(std::toupper(c)); }
	);

	inc_guard_str += "_H";

	return inc_guard_str;
}

template<typename T>
static void write_system_impl_fn_decl(
	ecsact::codegen_plugin_context& ctx,
	T                               id
) {
	using ecsact::cc_lang_support::c_identifier;

	auto        decl_id = ecsact_id_cast<ecsact_decl_id>(id);
	std::string full_name = ecsact_meta_decl_full_name(decl_id);
	if(full_name.empty()) {
		return;
	}

	auto c_impl_fn_name = c_identifier(full_name);

	ctx.writef("\n");

	auto assoc_ids = ecsact::meta::system_assoc_ids(id);

	for(auto i = 0; assoc_ids.size() > i; ++i) {
		auto c_impl_assoc_id_name = std::format("{}__{}", c_impl_fn_name, i);
		ctx.writef(
			"ECSACT_EXTERN\n"
			"ECSACT_EXPORT(\"{0}\")\n"
			"const ecsact_system_assoc_id {0};\n",
			c_impl_assoc_id_name
		);
	}

	ctx.writef(
		"ECSACT_EXTERN\n"
		"ECSACT_EXPORT(\"{0}\")\n"
		"void {0}(struct ecsact_system_execution_context*);\n",
		c_impl_fn_name
	);
}

void ecsact_codegen_plugin(
	ecsact_package_id          package_id,
	ecsact_codegen_write_fn_t  write_fn,
	ecsact_codegen_report_fn_t report_fn
) {
	using namespace std::string_literals;

	ecsact::codegen_plugin_context ctx{package_id, 0, write_fn, report_fn};
	const auto inc_guard_str = make_package_inc_guard_str(package_id);

	ctx.writef(GENERATED_FILE_DISCLAIMER);
	ctx.writef("#ifndef {}\n", inc_guard_str);
	ctx.writef("#define {}\n\n", inc_guard_str);

	ctx.writef("#include \"ecsact/runtime/common.h\"\n\n");

	for(auto action_id : get_action_ids(ctx.package_id)) {
		write_system_impl_fn_decl(ctx, action_id);
	}

	for(auto sys_id : get_system_ids(ctx.package_id)) {
		write_system_impl_fn_decl(ctx, sys_id);
	}

	ctx.writef("\n");
	ctx.writef("#endif // {}\n", inc_guard_str);
}
