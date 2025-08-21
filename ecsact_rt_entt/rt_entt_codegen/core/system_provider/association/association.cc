#include "association.hh"

#include <map>

#include "ecsact/lang-support/lang-cc.hh"
#include "rt_entt_codegen/shared/system_util.hh"
#include "rt_entt_codegen/shared/util.hh"
#include "ecsact/runtime/meta.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"

using capability_t =
	std::unordered_map<ecsact_component_like_id, ecsact_system_capability>;

using namespace ecsact::rt_entt_codegen::core;

auto provider::association::context_function_header(
	ecsact::codegen_plugin_context&                   ctx,
	const ecsact::rt_entt_codegen::core::common_vars& names
) -> void {
}

auto provider::association::after_make_view_or_group(
	ecsact::codegen_plugin_context&                   ctx,
	const ecsact::rt_entt_codegen::core::common_vars& names
) -> void {
}

auto provider::association::context_function_other(
	ecsact::codegen_plugin_context&                   ctx,
	const ecsact::rt_entt_codegen::core::common_vars& names
) -> handle_exclusive_provide {
	context_other_impl(ctx, sys_like_id, system_details);
	return HANDLED;
}

auto provider::association::pre_entity_iteration(
	ecsact::codegen_plugin_context&                   ctx,
	const ecsact::rt_entt_codegen::core::common_vars& names
) -> void {
	print_other_contexts(ctx, names);
}

auto provider::association::pre_exec_system_impl(
	ecsact::codegen_plugin_context&                   ctx,
	const ecsact::rt_entt_codegen::core::common_vars& names
) -> void {
}

auto provider::association::system_impl(
	ecsact::codegen_plugin_context&                   ctx,
	const ecsact::rt_entt_codegen::core::common_vars& names
) -> handle_exclusive_provide {
	return NOT_HANDLED;
}

auto provider::association::print_other_contexts(
	ecsact::codegen_plugin_context&                   ctx,
	const ecsact::rt_entt_codegen::core::common_vars& names
) -> void {
}

using ecsact::rt_entt_codegen::util::method_printer;

auto provider::association::print_other_ctx_action(
	ecsact::codegen_plugin_context& ctx
) -> void {
}

auto provider::association::print_other_ctx_add(
	ecsact::codegen_plugin_context&                            ctx,
	const capability_t&                                        other_caps,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
) -> void {
}

auto provider::association::print_other_ctx_remove(
	ecsact::codegen_plugin_context&                            ctx,
	const capability_t&                                        other_caps,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	const std::string&                                         view_type_name
) -> void {
}

auto provider::association::print_other_ctx_get(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	const std::string&                                         view_type_name
) -> void {
}

auto provider::association::print_other_ctx_update(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	const std::string&                                         view_type_name
) -> void {
}

auto provider::association::print_other_ctx_has(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
) -> void {
}

auto provider::association::print_other_ctx_generate(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
) -> void {
}

auto provider::association::print_other_ctx_parent(
	ecsact::codegen_plugin_context&                        ctx,
	const ecsact::rt_entt_codegen::system_like_id_variant& sys_like_id
) -> void {
}

auto provider::association::print_other_ctx_other(
	ecsact::codegen_plugin_context&                            ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
) -> void {
}
