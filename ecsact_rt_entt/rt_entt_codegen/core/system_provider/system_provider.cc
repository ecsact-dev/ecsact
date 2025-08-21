#include "system_provider.hh"

using namespace ecsact::rt_entt_codegen::core::provider;

system_provider::system_provider(system_like_id_variant id)
	: sys_like_id(id)
	, system_details(ecsact_entt_system_details::from_system_like(sys_like_id)) {
	assert(sys_like_id != system_like_id_variant{});
}

ecsact::rt_entt_codegen::core::provider::system_provider::~system_provider() =
	default;

auto system_provider::initialization(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> void {
}

auto system_provider::before_make_view_or_group(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names,
	std::vector<std::string>&       additional_view_components
) -> void {
}

auto system_provider::after_make_view_or_group(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> void {
}

auto system_provider::context_function_header(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> void {
}

auto system_provider::context_function_action(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	return NOT_HANDLED;
}

auto system_provider::context_function_add(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	return NOT_HANDLED;
}

auto system_provider::context_function_remove(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	return NOT_HANDLED;
}

auto system_provider::context_function_get(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	return NOT_HANDLED;
}

auto system_provider::context_function_update(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	return NOT_HANDLED;
}

auto system_provider::context_function_has(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	return NOT_HANDLED;
}

auto system_provider::context_function_generate(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	return NOT_HANDLED;
}

auto system_provider::context_function_parent(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	return NOT_HANDLED;
}

auto system_provider::context_function_other(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	return NOT_HANDLED;
}

auto system_provider::context_function_stream_toggle(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	return NOT_HANDLED;
}

auto system_provider::pre_entity_iteration(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> void {
}

auto system_provider::provide_context_init(
	ecsact::codegen_plugin_context&                   ctx,
	const ecsact::rt_entt_codegen::core::common_vars& names,
	std::string_view                                  context_type_name
) -> handle_exclusive_provide {
	return NOT_HANDLED;
}

auto system_provider::entity_iteration(
	ecsact::codegen_plugin_context&                   ctx,
	ecsact_system_like_id                             sys_like_id,
	const ecsact::rt_entt_codegen::core::common_vars& names,
	std::function<void()>                             iter_func
) -> handle_exclusive_provide {
	return NOT_HANDLED;
}

auto system_provider::pre_exec_system_impl(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> void {
}

auto system_provider::pre_exec_system_impl_context_init(
	ecsact::codegen_plugin_context&                   ctx,
	const ecsact::rt_entt_codegen::core::common_vars& names,
	std::string_view                                  context_type_name
) -> void {
}

auto system_provider::system_impl(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	return NOT_HANDLED;
}

auto system_provider::post_exec_system_impl(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> void {
}

auto system_provider::post_iteration(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> void {
}
