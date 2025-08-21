#include "core.hh"

#include <ranges>
#include "ecsact/runtime/meta.hh"
#include "ecsact/lang-support/lang-cc.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"
#include "rt_entt_codegen/shared/util.hh"

template<typename CompositeID, typename Fn>
static auto for_each_entity_field(CompositeID composite_id, Fn&& fn) {
	for(auto field_id : ecsact::meta::get_field_ids(composite_id)) {
		auto field_type = ecsact_meta_field_type(
			ecsact_id_cast<ecsact_composite_id>(composite_id),
			field_id
		);

		const auto is_entity_field = //
			field_type.kind == ECSACT_TYPE_KIND_BUILTIN &&
			field_type.type.builtin == ECSACT_ENTITY_TYPE;

		if(!is_entity_field) {
			continue;
		}

		auto entity_field_name = std::string{ecsact_meta_field_name(
			ecsact_id_cast<ecsact_composite_id>(composite_id),
			field_id
		)};

		fn(entity_field_name);
	}
}

static auto print_check_add_component_error_template_specialization(
	ecsact::codegen_plugin_context&                     ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_details& details,
	ecsact_component_id                                 component_id
) -> void {
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::cpp_codegen_plugin_util::method_printer;
	using ecsact::rt_entt_codegen::util::decl_cpp_ident;

	auto cpp_component_ident = decl_cpp_ident(component_id);

	const auto method_name =
		"ecsact::entt::check_add_component_error<" + cpp_component_ident + ">";

	ctx.write("template<>\n");

	auto printer = //
		method_printer{ctx, method_name}
			.parameter("ecsact::entt::registry_t&", "registry")
			.parameter("::ecsact::entt::entity_id", "entity")
			.parameter(cpp_component_ident + " const&", "component")
			.return_type("ecsact_add_error");

	for_each_entity_field(component_id, [&](auto field_name) {
		auto field_var = "ecsact::entt::entity_id{component." + field_name + "}";
		block(ctx, "if(!registry.valid(" + field_var + "))", [&] {
			ctx.write("return ECSACT_ADD_ERR_ENTITY_INVALID;\n");
		});
	});

	ctx.write("return ECSACT_ADD_OK;");
}

static auto print_check_update_component_error_template_specialization(
	ecsact::codegen_plugin_context&                     ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_details& details,
	ecsact_component_id                                 component_id
) -> void {
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::cpp_codegen_plugin_util::method_printer;
	using ecsact::rt_entt_codegen::util::decl_cpp_ident;

	auto cpp_component_ident = decl_cpp_ident(component_id);

	const auto method_name =
		"ecsact::entt::check_update_component_error<" + cpp_component_ident + ">";

	ctx.write("template<>\n");

	auto printer = //
		method_printer{ctx, method_name}
			.parameter("ecsact::entt::registry_t&", "registry")
			.parameter("::ecsact::entt::entity_id", "entity")
			.parameter(cpp_component_ident + " const&", "component")
			.return_type("ecsact_update_error");

	for_each_entity_field(component_id, [&](auto field_name) {
		auto field_var = "ecsact::entt::entity_id{component." + field_name + "}";
		block(ctx, "if(!registry.valid(" + field_var + "))", [&] {
			ctx.write("return ECSACT_UPDATE_ERR_ENTITY_INVALID;\n");
		});
	});

	ctx.write("return ECSACT_UPDATE_OK;");
}

static auto print_check_action_error_template_specialization(
	ecsact::codegen_plugin_context&                     ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_details& details,
	ecsact_action_id                                    action_id
) -> void {
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::cpp_codegen_plugin_util::method_printer;
	using ecsact::rt_entt_codegen::util::decl_cpp_ident;

	auto cpp_action_ident = decl_cpp_ident(action_id);

	const auto method_name =
		"ecsact::entt::check_action_error<" + cpp_action_ident + ">";

	ctx.write("template<>\n");

	auto printer = //
		method_printer{ctx, method_name}
			.parameter("ecsact::entt::registry_t&", "registry")
			.parameter(cpp_action_ident + " const&", "action")
			.return_type("ecsact_execute_systems_error");

	ctx.write("auto err = ECSACT_EXEC_SYS_OK;\n");

	for_each_entity_field(action_id, [&](auto field_name) {
		auto field_var = "ecsact::entt::entity_id{action." + field_name + "}";
		block(ctx, "if(!registry.valid(" + field_var + "))", [&] {
			ctx.write("return ECSACT_EXEC_SYS_ERR_ACTION_ENTITY_INVALID;\n");
		});
	});

	ctx.write("return err;\n");
}

auto ecsact::rt_entt_codegen::core::print_check_error_template_specializations(
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void {
	for(auto comp_id : details.all_components) {
		print_check_add_component_error_template_specialization(
			ctx,
			details,
			comp_id
		);
	}

	for(auto comp_id : details.all_components) {
		print_check_update_component_error_template_specialization(
			ctx,
			details,
			comp_id
		);
	}

	for(auto action_id : details.all_actions) {
		print_check_action_error_template_specialization(ctx, details, action_id);
	}
}
