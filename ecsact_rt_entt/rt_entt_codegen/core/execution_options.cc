#include "core.hh"

#include "ecsact/lang-support/lang-cc.hh"
#include "rt_entt_codegen/shared/util.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"

inline auto print_static_maps(
	ecsact::codegen_plugin_context&                     ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_details& details
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::meta::decl_full_name;

	block(
		ctx,
		"static const auto execution_add_fns ="
		"std::unordered_map<ecsact_component_like_id, "
		"decltype(&ecsact_add_component)>\n",
		[&] {
			for(auto component_id : details.all_components) {
				auto type_name = cpp_identifier(decl_full_name(component_id));
				ctx.write(
					"{",
					"ecsact_id_cast<ecsact_component_like_id>(",
					type_name,
					"::id), ",
					"&ecsact::entt::wrapper::core::add_component_exec_options",
					"<::",
					type_name,
					"> },\n"
				);
			}
		}
	);
	ctx.write(";\n");

	block(
		ctx,
		"static const auto execution_update_fns ="
		"std::unordered_map<ecsact_component_like_id, "
		"decltype(&ecsact_update_component)>\n",
		[&] {
			for(auto component_id : details.all_components) {
				auto type_name = cpp_identifier(decl_full_name(component_id));
				auto field_count = ecsact_meta_count_fields(
					ecsact_id_cast<ecsact_composite_id>(component_id)
				);
				if(field_count == 0) {
					continue;
				}
				ctx.write(
					"{",
					"ecsact_id_cast<ecsact_component_like_id>(",
					type_name,
					"::id), ",
					"&ecsact::entt::wrapper::core::update_component_exec_options"
					"<::",
					type_name,
					"> },\n"
				);
			}
		}
	);
	ctx.write(";\n");

	block(
		ctx,
		"static const auto execution_remove_fns= "
		"std::unordered_map<ecsact_component_like_id, "
		"decltype(&ecsact_remove_component)>\n",
		[&] {
			for(auto component_id : details.all_components) {
				auto type_name = cpp_identifier(decl_full_name(component_id));
				ctx.write(
					"{",
					"ecsact_id_cast<ecsact_component_like_id>(",
					type_name,
					"::id), ",
					"&ecsact::entt::wrapper::core::remove_component_exec_options",
					"<::",
					type_name,
					"> },\n"
				);
			}
		}
	);
	ctx.write(";\n");

	block(
		ctx,
		"static const auto action_error_fns= "
		"std::unordered_map<ecsact_action_id, "
		"decltype(&ecsact::entt::wrapper::core::check_action_error_t)>\n",
		[&] {
			for(auto action_id : details.all_actions) {
				auto type_name = cpp_identifier(decl_full_name(action_id));
				ctx.write(
					"{",
					"ecsact_id_cast<ecsact_action_id>(",
					type_name,
					"::id), ",
					"&ecsact::entt::wrapper::core::check_action_error",
					"<::",
					type_name,
					"> },\n"
				);
			}
		}
	);

	ctx.write(";\n");
}

auto ecsact::rt_entt_codegen::core::print_execution_options(
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::meta::decl_full_name;
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "handle_execution_options"}
			.parameter("ecsact_registry_id", "registry_id")
			.parameter("const ecsact_execution_options&", "options")
			.return_type("ecsact_execute_systems_error");

	print_static_maps(ctx, details);
	ctx.write("auto& reg = ecsact::entt::get_registry(registry_id);\n");

	block(ctx, "for(int i = 0; i < options.actions_length; i++)", [&] {
		ctx.write("auto action = options.actions[i];\n");
		ctx.write(
			"auto err = action_error_fns.at(action.action_id)(registry_id, "
			"action.action_data);\n"
		);
		block(ctx, "if(err != ECSACT_EXEC_SYS_OK)", [&] {
			ctx.write("return err;\n");
		});
	});

	block(ctx, "for(int i = 0; i < options.create_entities_length; i++)", [&] {
		ctx.write("auto entity = ecsact::entt::entity_id(reg.create());\n");
		ctx.write(
			"reg.template emplace<ecsact::entt::detail::created_entity>(entity, "
			"options.create_entities[i]);\n"
		);
		block(
			ctx,
			"for(int j = 0; j < options.create_entities_components_length[i]; "
			"j++)",
			[&] {
				ctx.write(
					"auto& component = options.create_entities_components[i][j];\n"
				);
				ctx.write(
					"execution_add_fns.at(ecsact_id_cast<ecsact_component_like_id>("
					"component.component_id))(registry_id, "
					"entity, "
					"component.component_id, component.component_data);\n"
				);
			}
		);
	});

	block(ctx, "for(int i = 0; i < options.add_components_length; i++)", [&] {
		ctx.write("auto& component = options.add_components[i];\n");
		ctx.write("auto entity = options.add_components_entities[i];\n\n");

		ctx.write(
			"execution_add_fns.at(ecsact_id_cast<ecsact_component_like_id>("
			"component.component_id))(registry_id, "
			"ecsact::entt::entity_id(entity), "
			"component.component_id, component.component_data);\n"
		);
	});

	block(ctx, "for(int i = 0; i < options.update_components_length; i++)", [&] {
		ctx.write("auto& component = options.update_components[i];\n");
		ctx.write("auto entity = options.update_components_entities[i];\n\n");

		ctx.write(
			"execution_update_fns.at(ecsact_id_cast<ecsact_component_like_id>("
			"component.component_id))(registry_id, "
			"ecsact::entt::entity_id(entity), "
			"component.component_id, component.component_data, nullptr);\n"
		);
	});

	block(ctx, "for(int i = 0; i < options.remove_components_length; i++)", [&] {
		ctx.write("auto& component_id = options.remove_components[i];\n");
		ctx.write("auto entity = options.remove_components_entities[i];\n\n");

		ctx.write(
			"execution_remove_fns.at(ecsact_id_cast<ecsact_component_like_id>("
			"component_id))(registry_id, "
			"ecsact::entt::entity_id(entity), "
			"component_id, nullptr);\n\n"
		);
	});

	block(ctx, "for(int i = 0; i < options.destroy_entities_length; i++)", [&] {
		ctx.write("auto entity = options.destroy_entities[i];\n");
		ctx.write("reg.destroy(ecsact::entt::entity_id(entity));\n");
		ctx.write(
			"reg.template "
			"emplace<ecsact::entt::detail::destroyed_entity>(ecsact::entt::entity_id("
			"entity));\n"
		);
	});

	ctx.write("return ECSACT_EXEC_SYS_OK;\n");
}
