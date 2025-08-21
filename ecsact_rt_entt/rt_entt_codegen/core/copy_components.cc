#include "core.hh"

#include "ecsact/lang-support/lang-cc.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"
#include "rt_entt_codegen/shared/util.hh"

auto ecsact::rt_entt_codegen::core::print_copy_components(
	ecsact::codegen_plugin_context&                     ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_details& details
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::meta::decl_full_name;
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "ecsact::entt::copy_components"}
			.parameter("const ::entt::registry&", "src")
			.parameter("::entt::registry&", "dst")
			.return_type("void");

	for(auto comp_id : details.all_components) {
		const auto cpp_comp_name = cpp_identifier(decl_full_name(comp_id));
		const auto is_tag = ecsact::meta::get_field_ids(comp_id).empty();

		if(is_tag) {
			block(
				ctx,
				std::format("for(auto entity : src.view<{}>())", cpp_comp_name),
				[&] { ctx.writef("dst.emplace<{}>(entity);\n", cpp_comp_name); }
			);
		} else {
			block(
				ctx,
				std::format(
					"for(auto&& [entity, comp] : src.view<{}>().each())",
					cpp_comp_name
				),
				[&] { ctx.writef("dst.emplace<{}>(entity, comp);\n", cpp_comp_name); }
			);
		}
	}

	for(auto sys_id : details.all_systems) {
		const auto system_name = cpp_identifier(decl_full_name(sys_id));
		const auto pending_lazy_exec_struct = std::format(
			"::ecsact::entt::detail::pending_lazy_execution<::{}>",
			system_name
		);
		block(
			ctx,
			std::format(
				"for(auto entity : src.view<{}>())",
				pending_lazy_exec_struct
			),
			[&] {
				ctx.writef("dst.emplace<{}>(entity);\n", pending_lazy_exec_struct);
			}
		);
	}
}
