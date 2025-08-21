#include "system_ctx_functions.hh"

#include "ecsact/runtime/meta.hh"
#include "ecsact/lang-support/lang-cc.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"
#include "rt_entt_codegen/shared/util.hh"

using ecsact::cc_lang_support::cpp_identifier;
using ecsact::cpp_codegen_plugin_util::block;
using ecsact::meta::decl_full_name;
using ecsact::rt_entt_codegen::util::is_transient_component;
using ecsact::rt_entt_codegen::util::make_id_map_type;

auto ecsact::rt_entt_codegen::core::provider::context_action_impl(
	ecsact::codegen_plugin_context& ctx,
	const system_like_id_variant&   sys_like_id
) -> void {
	if(sys_like_id.is_action()) {
		auto action_name = cpp_identifier(decl_full_name(sys_like_id));
		ctx.write(
			"\t#ifdef TRACY_ENABLE\n",
			"ZoneScopedNC(\"context_action ",
			action_name,
			"\", tracy::Color::Blue);\n",
			"#endif\n"
		);

		ctx.write(
			"*static_cast<",
			action_name,
			"*>(out_action_data) = *static_cast<const ",
			action_name,
			"*>(action_data);\n"
		);
	} else {
		// TODO(Kelwan): Trying to access .action() without a valid action
		ctx.write("\n");
	}
}

auto ecsact::rt_entt_codegen::core::provider::context_add_impl(
	ecsact::codegen_plugin_context& ctx,
	const capability_t&             sys_caps
) -> void {
	auto adds_comps = std::vector<ecsact_component_like_id>{};

	for(auto&& [comp_id, sys_cap] : sys_caps) {
		if(is_transient_component(ctx.package_id, comp_id)) {
			continue;
		}
		if((ECSACT_SYS_CAP_ADDS & sys_cap) == ECSACT_SYS_CAP_ADDS) {
			adds_comps.push_back(comp_id);
		}
	}

	if(adds_comps.empty()) {
		// TODO(Kelwan): Handle unexpected behaviour
		return;
	}

	if(adds_comps.size() == 1) {
		const auto& comp_id = adds_comps.front();
		auto        type_name = cpp_identifier(decl_full_name(comp_id));
		ctx.write(
			"wrapper::dynamic::context_add<::",
			type_name,
			">(this, ecsact_id_cast<ecsact_component_like_id>(",
			type_name,
			"::id),",
			"component_data); \n"
		);
		return;
	}

	block(
		ctx,
		"static const auto add_fns = "
		"std::unordered_map<ecsact_component_like_id, "
		"decltype(&ecsact_system_execution_context_add)>",
		[&] {
			for(int i = 0; i < adds_comps.size(); ++i) {
				const auto comp_id = adds_comps[i];
				auto       type_name = cpp_identifier(decl_full_name(comp_id));
				ctx.write(
					"{",
					"ecsact_id_cast<ecsact_component_like_id>(",
					type_name,
					"::id), ",
					"&wrapper::dynamic::context_add<::",
					type_name,
					"> },"
				);
			}
		}
	);

	ctx.write(";\n");

	ctx.write("add_fns.at(component_id)(this, component_id, component_data);\n");
}

auto ecsact::rt_entt_codegen::core::provider::context_remove_impl(
	ecsact::codegen_plugin_context&                            ctx,
	const capability_t&                                        sys_caps,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	const std::string&                                         view_type_name
) -> void {
	auto remove_comps = std::vector<ecsact_component_like_id>{};

	for(auto&& [comp_id, sys_cap] : sys_caps) {
		if(is_transient_component(ctx.package_id, comp_id)) {
			continue;
		}
		if((ECSACT_SYS_CAP_REMOVES & sys_cap) == ECSACT_SYS_CAP_REMOVES) {
			remove_comps.push_back(comp_id);
		}
	}

	if(remove_comps.size() == 0) {
		// TODO(Kelwan): Handle unexpected behaviour
		return;
	}

	if(remove_comps.size() == 1) {
		const auto& comp_id = remove_comps.front();

		auto type_name = cpp_identifier(decl_full_name(comp_id));
		ctx.write(
			"wrapper::dynamic::context_remove<::",
			type_name,
			">(this, ecsact_id_cast<ecsact_component_like_id>(",
			type_name,
			"::id), nullptr, *view);\n"
		);
		return;
	}
	ctx.write(std::format(
		"using remove_fn_t = void (*)(ecsact_system_execution_context*, "
		"ecsact_component_like_id, const void*, {}_t&);\n",
		view_type_name
	));

	ctx.write("static const auto remove_fns = []()\n");

	block(ctx, "", [&] {
		ctx.write(
			"auto result = std::unordered_map<ecsact_component_like_id, "
			"remove_fn_t>{};\n"
		);
		for(const auto comp_id : details.removable_comps) {
			auto type_name = cpp_identifier(decl_full_name(comp_id));
			ctx.write(
				"result[ecsact_id_cast<ecsact_component_like_id>(",
				type_name,
				"::id)] = &wrapper::dynamic::context_remove<",
				type_name,
				">;\n"
			);
		}

		ctx.write("return result;\n");
	});
	ctx.write("();\n");
	ctx.write("remove_fns.at(component_id)(this, component_id, nullptr, *view);\n"
	);
}

auto ecsact::rt_entt_codegen::core::provider::context_get_impl(
	ecsact::codegen_plugin_context&                            ctx,
	const system_like_id_variant&                              sys_like_id,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	const std::string&                                         view_type_name
) -> void {
	if(details.readable_comps.size() == 0) {
		return;
	}

	std::vector<ecsact_component_like_id> get_components;

	for(auto comp_id : details.readable_comps) {
		if(is_transient_component(ctx.package_id, comp_id)) {
			continue;
		}
		get_components.insert(get_components.end(), comp_id);
	}

	if(get_components.size() == 0) {
		return;
	}
	auto system_name = decl_full_name(sys_like_id);
	ctx.write(
		"#ifdef TRACY_ENABLE\n"
		"\tZoneScopedNC(\"context_get ",
		system_name,
		"\", tracy::Color::Purple);\n",
		"#endif\n"
	);

	// Shortcut - ignore component ID because we only have 1
	if(details.get_comps.size() == 1 && details.readable_comps.size() == 1) {
		auto comp_id = *details.get_comps.begin();
		auto comp_name = decl_full_name(comp_id);
		auto type_name = cpp_identifier(comp_name);

		ctx.write(
			"assert(ecsact_id_cast<ecsact_component_like_id>(::",
			type_name,
			"::id) == component_id);\n"
		);
		ctx.write(
			"*static_cast<::",
			type_name,
			"*>(out_component_data) = view->get<::",
			type_name,
			">(entity);"
		);
		return;
	}

	ctx.write(std::format(
		"using get_fn_t = void (*)(ecsact_system_execution_context*, "
		"ecsact_component_like_id, void *, const void*, {}_t&);\n",
		view_type_name
	));

	ctx.write("static const auto get_fns = []()\n");

	block(ctx, "", [&] {
		ctx.writef(
			"auto result = {}{{}};\n",
			make_id_map_type<ecsact_component_like_id>("get_fn_t")
		);
		for(const auto comp_id : details.readable_comps) {
			auto type_name = cpp_identifier(decl_full_name(comp_id));
			ctx.write(
				"result[ecsact_id_cast<ecsact_component_like_id>(",
				type_name,
				"::id)] = &wrapper::dynamic::context_get<",
				type_name,
				">;\n"
			);
		}

		ctx.write("return result;\n");
	});
	ctx.write("();\n");

	ctx.write(
		"get_fns[component_id]"
		"(this, component_id, out_component_data, nullptr, *view"
		");\n"
	);
}

auto ecsact::rt_entt_codegen::core::provider::context_update_impl(
	ecsact::codegen_plugin_context&                            ctx,
	const system_like_id_variant&                              sys_like_id,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	const std::string&                                         view_type_name
) -> void {
	if(details.writable_comps.size() == 0) {
		return;
	}

	auto system_name = decl_full_name(sys_like_id);
	ctx.write(
		"#ifdef TRACY_ENABLE\n",
		"\tZoneScopedNC(\"context_update ",
		system_name,
		"\", tracy::Color::Red);\n",
		"#endif\n"
	);

	if(details.writable_comps.size() == 1) {
		const auto& comp_id = *details.writable_comps.begin();
		auto        comp_name = decl_full_name(comp_id);
		auto        type_name = cpp_identifier(comp_name);

		ctx.write(
			"wrapper::dynamic::context_update<::",
			type_name,
			">(this, ecsact_id_cast<ecsact_component_like_id>(",
			type_name,
			"::id),",
			"component_data, nullptr, *view); \n"
		);
		return;
	}

	ctx.write(std::format(
		"using update_fn_t = void (*)(ecsact_system_execution_context*, "
		"ecsact_component_like_id, const void *, const void*, {}_t&);\n",
		view_type_name
	));

	ctx.write("static const auto update_fns = []()\n");

	block(ctx, "", [&] {
		ctx.writef(
			"auto result = {}{{}};\n",
			make_id_map_type<ecsact_component_like_id>("update_fn_t")
		);
		for(const auto comp_id : details.writable_comps) {
			auto type_name = cpp_identifier(decl_full_name(comp_id));
			ctx.write(
				"result[ecsact_id_cast<ecsact_component_like_id>(",
				type_name,
				"::id)] = &wrapper::dynamic::context_update<",
				type_name,
				">;\n"
			);
		}

		ctx.write("return result;\n");
	});
	ctx.write("();\n");

	ctx.write(
		"update_fns[component_id]"
		"(this, component_id, component_data, nullptr, *view);\n"
	);
}

auto ecsact::rt_entt_codegen::core::provider::context_has_impl(
	ecsact::codegen_plugin_context&                            ctx,
	const system_like_id_variant&                              sys_like_id,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
) -> void {
	if(details.writable_comps.size() == 0) {
		ctx.write("return false;");
		return;
	}

	auto system_name = decl_full_name(sys_like_id);
	ctx.write(
		"#ifdef TRACY_ENABLE\n",
		"\tZoneScopedNC(\"context_has ",
		system_name,
		"\", tracy::Color::Gray);\n",
		"#endif\n"
	);

	if(details.writable_comps.size() == 1) {
		const auto& comp_id = *details.writable_comps.begin();
		auto        type_name = cpp_identifier(decl_full_name(comp_id));
		auto        comp_name = decl_full_name(comp_id);

		ctx.write(
			"return wrapper::dynamic::context_has<::",
			type_name,
			">(this, ecsact_id_cast<ecsact_component_like_id>(",
			type_name,
			"::id), nullptr);\n"
		);
	}
	block(
		ctx,
		"static const auto has_fns = "
		"std::unordered_map<ecsact_component_like_id, "
		"decltype(&ecsact_system_execution_context_has)>",
		[&] {
			for(const auto comp_id : details.readable_comps) {
				auto type_name = cpp_identifier(decl_full_name(comp_id));
				ctx.write(
					"{ecsact_id_cast<ecsact_component_like_id>(",
					type_name,
					"::id), &wrapper::dynamic::context_has<",
					type_name,
					"> },"
				);
			}
		}
	);
	ctx.write(";\n");

	ctx.write("return has_fns.at(component_id)(this, component_id, nullptr);\n");
}

auto ecsact::rt_entt_codegen::core::provider::context_generate_impl(
	ecsact::codegen_plugin_context&                            ctx,
	const system_like_id_variant&                              sys_like_id,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
) -> void {
	if(details.generate_comps.size() == 0) {
		// TODO (Kelwan): Handle undefined behaviour
		return;
	}

	auto system_name = decl_full_name(sys_like_id);
	ctx.write(
		"#ifdef TRACY_ENABLE\n",
		"\tZoneScopedNC(\"context_generate ",
		system_name,
		"\", tracy::Color::Yellow);\n",
		"#endif\n"
	);

	block(
		ctx,
		"static const auto generate_fns = "
		"std::unordered_map<ecsact_component_id, "
		"void(*)(ecsact_system_execution_context*, ecsact_component_id, const "
		"void*, const void*, ecsact::entt::entity_id)>",
		[&] {
			for(const auto& component : details.generate_comps) {
				for(const auto& [comp_id, requirements] : component) {
					auto comp_name = decl_full_name(comp_id);
					auto type_name = cpp_identifier(comp_name);

					ctx.write(
						"{",
						type_name,
						"::id, &wrapper::dynamic::context_generate_add<",
						type_name,
						"> },"
					);
				}
			}
		}
	);
	ctx.write(";\n");

	// NOTE(Kelwan): Multiple generates blocks are allowed in Ecsact systems but
	// currently the interpreter won't allow this. More testing required after the
	// issue is resolved https://github.com/ecsact-dev/ecsact_interpret/issues/185
	ctx.write("auto entity = registry->create();\n");

	ctx.write(
		"registry->template emplace<ecsact::entt::detail::created_entity>(entity, "
		"ecsact_generated_entity);\n"
	);

	block(ctx, "for(int i = 0; i < component_count; ++i)", [&] {
		ctx.write("const auto component_id = component_ids[i];\n");
		ctx.write("const void* component_data = components_data[i];\n");

		ctx.write(
			"generate_fns.at(component_id)(this, component_id, "
			"component_data, nullptr, entity);\n"
		);
	});
}

auto ecsact::rt_entt_codegen::core::provider::context_stream_toggle_impl(
	ecsact::codegen_plugin_context&                            ctx,
	const system_like_id_variant&                              sys_like_id,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
) -> void {
	auto stream_comps = details.stream_comps;
	if(stream_comps.empty()) {
		// TODO(Kelwan): Handle unexpected behaviour
		return;
	}

	auto system_name = decl_full_name(sys_like_id);
	ctx.write(
		"#ifdef TRACY_ENABLE\n",
		"\tZoneScopedNC(\"context_stream_toggle ",
		system_name,
		"\", tracy::Color::Green);\n",
		"#endif\n"
	);

	if(stream_comps.size() == 1) {
		const auto& comp_id = stream_comps.begin();
		auto        type_name = cpp_identifier(decl_full_name(*comp_id));

		ctx.write(
			"wrapper::dynamic::context_stream_toggle<::",
			type_name,
			">(this, ecsact_id_cast<ecsact_component_id>(",
			type_name,
			"::id), streaming_enabled, indexed_fields); \n"
		);
		return;
	}

	block(
		ctx,
		"static const auto toggle_fns = "
		"std::unordered_map<ecsact_component_id, "
		"decltype(&ecsact_system_execution_context_stream_toggle)>",
		[&] {
			for(const auto comp_id : stream_comps) {
				auto type_name = cpp_identifier(decl_full_name(comp_id));
				ctx.write(
					"{",
					type_name,
					"::id, ",
					"&wrapper::dynamic::context_stream_toggle<::",
					type_name,
					"> },\n"
				);
			}
		}
	);
	ctx.writef(";\n");

	ctx.writef( //
		"return toggle_fns.at(component_id)("
		"this, component_id, streaming_enabled, indexed_fields"
		");\n"
	);
}

auto ecsact::rt_entt_codegen::core::provider::context_parent_impl(
	ecsact::codegen_plugin_context& ctx,
	const system_like_id_variant&   sys_like_id
) -> void {
	ctx.write("return this->parent_ctx;\n");
}

auto ecsact::rt_entt_codegen::core::provider::context_other_impl(
	ecsact::codegen_plugin_context&                            ctx,
	const system_like_id_variant&                              sys_like_id,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
) -> void {
}
