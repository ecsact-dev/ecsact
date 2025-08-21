#include "core.hh"

#include "ecsact/lang-support/lang-cc.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"
#include "rt_entt_codegen/shared/util.hh"

auto ecsact::rt_entt_codegen::core::print_hash_registry(
	ecsact::codegen_plugin_context&                     ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_details& details
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::meta::decl_full_name;
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "ecsact::entt::hash_registry"}
			.parameter("const ::entt::registry&", "reg")
			.return_type("std::uint64_t");

	ctx.writef("auto* state = XXH3_createState();\n");
	ctx.writef("if(!state) {{ return 0; }}\n");
	ctx.writef("XXH3_64bits_reset(state);\n");

	// XXH3_64bits_update(state, buffer, count);

	block(
		ctx,
		"for(auto&& [entity] : reg.template storage<::entt::entity>()->each())",
		[&] {
			ctx.writef("XXH3_64bits_update(state, &entity, sizeof(entity));\n");
			ctx.writef(
				"auto has_states = std::array<bool, {}>{{\n",
				details.all_components.size() + details.all_systems.size()
			);
			for(auto comp_id : details.all_components) {
				const auto cpp_comp_name = cpp_identifier(decl_full_name(comp_id));
				ctx.writef("\treg.all_of<{}>(entity),\n", cpp_comp_name);
			}

			for(auto sys_id : details.all_systems) {
				const auto system_name = cpp_identifier(decl_full_name(sys_id));
				const auto pending_lazy_exec_struct = std::format(
					"::ecsact::entt::detail::pending_lazy_execution<::{}>",
					system_name
				);
				ctx.writef("\treg.all_of<{}>(entity),\n", pending_lazy_exec_struct);
			}

			ctx.writef("\n}};\n");
			ctx.writef(
				"XXH3_64bits_update(state, has_states.data(), sizeof(bool) * "
				"has_states.size());\n"
			);
		}
	);

	ctx.writef("\n");

	for(auto comp_id : details.all_components) {
		const auto cpp_comp_name = cpp_identifier(decl_full_name(comp_id));
		if(ecsact::meta::get_field_ids(comp_id).empty()) {
			// tags are covered in the `has_states` above already
			continue;
		}

		block(
			ctx,
			std::format(
				"for(auto&& [entity, comp] : reg.view<{}>().each())",
				cpp_comp_name
			),
			[&] {
				auto field_ids = ecsact::meta::get_field_ids(comp_id);
				for(auto field_id : field_ids) {
					auto field_name = ecsact::meta::field_name(comp_id, field_id);
					ctx.writef(
						"XXH3_64bits_update(state, &comp.{0}, "
						"sizeof(decltype(comp.{0})));\n",
						field_name
					);
				}
			}
		);
		ctx.writef("\n");
	}

	ctx.writef("auto result = XXH3_64bits_digest(state);\n");
	ctx.writef("XXH3_freeState(state);\n");
	ctx.writef("return result;\n");
}
