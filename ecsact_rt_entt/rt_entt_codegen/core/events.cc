#include "core.hh"
#include "algorithm"

#include "ecsact/lang-support/lang-cc.hh"
#include "rt_entt_codegen/core/core.hh"
#include "rt_entt_codegen/shared/util.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"
#include "rt_entt_codegen/shared/comps_with_caps.hh"

static auto print_trigger_event_fn_call(
	ecsact::codegen_plugin_context& ctx,
	std::string                     event_name,
	std::string                     component_name
) {
	ctx.write(
		"::ecsact::entt::wrapper::core::_trigger_",
		event_name,
		"_component_event<::",
		component_name,
		">(registry_id, events_collector);\n"
	);
}

auto ecsact::rt_entt_codegen::core::print_trigger_ecsact_events_minimal( //
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::meta::decl_full_name;
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "trigger_component_events_minimal"}
			.parameter("ecsact_registry_id", "registry_id")
			.parameter(
				"ecsact::entt::detail::execution_events_collector&",
				"events_collector"
			)
			.return_type("void");

	ctx.write(
		"ecsact::entt::wrapper::core::_trigger_create_entity_events(registry_id, "
		"events_collector);\n"
	);

	for(auto comp_id : comps_with_caps(details, ECSACT_SYS_CAP_ADDS)) {
		auto type_name = cpp_identifier(decl_full_name(comp_id));
		print_trigger_event_fn_call(ctx, "init", type_name);
	}

	for(auto comp_id : comps_with_caps(details, ECSACT_SYS_CAP_WRITEONLY)) {
		auto type_name = cpp_identifier(decl_full_name(comp_id));
		print_trigger_event_fn_call(ctx, "update", type_name);
	}

	for(auto comp_id : comps_with_caps(details, ECSACT_SYS_CAP_REMOVES)) {
		auto type_name = cpp_identifier(decl_full_name(comp_id));
		print_trigger_event_fn_call(ctx, "remove", type_name);
	}

	ctx.write(
		"ecsact::entt::wrapper::core::_trigger_destroy_entity_events(registry_id, "
		"events_collector);\n"
	);
}

auto ecsact::rt_entt_codegen::core::print_trigger_ecsact_events_all( //
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::meta::decl_full_name;
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "trigger_component_events_all"}
			.parameter("ecsact_registry_id", "registry_id")
			.parameter(
				"ecsact::entt::detail::execution_events_collector&",
				"events_collector"
			)
			.return_type("void");

	ctx.write(
		"ecsact::entt::wrapper::core::_trigger_create_entity_events(registry_id, "
		"events_collector);\n"
	);

	for(auto component_id : details.all_components) {
		auto type_name = cpp_identifier(decl_full_name(component_id));
		print_trigger_event_fn_call(ctx, "init", type_name);
	}
	for(auto component_id : details.all_components) {
		auto type_name = cpp_identifier(decl_full_name(component_id));

		auto count =
			ecsact_meta_count_fields(ecsact_id_cast<ecsact_composite_id>(component_id)
			);

		if(count > 0) {
			print_trigger_event_fn_call(ctx, "update", type_name);
		}
	}
	for(auto component_id : details.all_components) {
		auto type_name = cpp_identifier(decl_full_name(component_id));
		print_trigger_event_fn_call(ctx, "remove", type_name);
	}

	ctx.write(
		"ecsact::entt::wrapper::core::_trigger_destroy_entity_events(registry_id, "
		"events_collector);\n"
	);
}

auto ecsact::rt_entt_codegen::core::print_cleanup_ecsact_component_events( //
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::meta::decl_full_name;
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "cleanup_component_events"}
			.parameter("ecsact_registry_id", "registry_id")
			.return_type("void");

	for(auto component_id : details.all_components) {
		auto type_name = cpp_identifier(decl_full_name(component_id));
		ctx.write(
			"ecsact::entt::wrapper::core::clear_component",
			"<::",
			type_name,
			">(registry_id);\n"
		);
	}
}
