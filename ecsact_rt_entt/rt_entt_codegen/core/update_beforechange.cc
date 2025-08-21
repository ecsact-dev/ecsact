#include "core.hh"

#include <format>

#include "ecsact/lang-support/lang-cc.hh"
#include "rt_entt_codegen/shared/util.hh"
#include "ecsact/runtime/common.h"
#include "ecsact/cpp_codegen_plugin_util.hh"

auto ecsact::rt_entt_codegen::core::print_update_all_beforechange_storage(
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void {
	using ecsact::cc_lang_support::c_identifier;
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::cpp_codegen_plugin_util::method_printer;
	using ecsact::meta::decl_full_name;

	auto printer = //
		method_printer{ctx, "update_all_beforechange_storage"}
			.parameter("ecsact_registry_id", "registry_id")
			.return_type("void");

	ctx.write("auto& reg = ecsact::entt::get_registry(registry_id);\n\n");

	for(auto comp_id : details.all_components) {
		if(ecsact::meta::get_field_ids(comp_id).empty()) {
			continue;
		}

		auto comp_name = c_identifier(decl_full_name((comp_id)));
		auto cpp_comp_name = cpp_identifier(decl_full_name(comp_id));
		auto comp_change_name = std::format( //
			"ecsact::entt::detail::exec_itr_beforechange_storage<{}>",
			cpp_comp_name
		);

		auto view_name = std::format("{}_view", comp_name);
		auto comp_list = std::format("{}, {}", cpp_comp_name, comp_change_name);

		ctx.write( //
			std::format("auto {} = reg.view<{}>();\n", view_name, comp_list)
		);

		block(ctx, std::format("for(auto entity: {})", view_name), [&]() {
			ctx.write(std::format(
				"ecsact::entt::wrapper::core::update_exec_itr_beforechange<{}>(entity, "
				"reg);\n",
				cpp_comp_name
			));
		});
	}
}
