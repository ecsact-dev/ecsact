#pragma once

#include <string>
#include <unordered_map>
#include "rt_entt_codegen/core/system_provider/system_provider.hh"
#include "rt_entt_codegen/core/sys_exec/sys_exec.hh"

namespace ecsact::rt_entt_codegen::core::provider {

class basic final : public system_provider {
public:
	using system_provider::system_provider;

	auto initialization(
		ecsact::codegen_plugin_context& ctx,
		const common_vars&              names
	) -> void final;

	auto context_function_header(
		ecsact::codegen_plugin_context& ctx,
		const common_vars&              names
	) -> void final;

	auto context_function_action(
		ecsact::codegen_plugin_context& ctx,
		const common_vars&              names
	) -> handle_exclusive_provide final;

	auto context_function_add(
		ecsact::codegen_plugin_context& ctx,
		const common_vars&              names
	) -> handle_exclusive_provide final;

	auto context_function_remove(
		ecsact::codegen_plugin_context& ctx,
		const common_vars&              names
	) -> handle_exclusive_provide final;

	auto context_function_get(
		ecsact::codegen_plugin_context& ctx,
		const common_vars&              names
	) -> handle_exclusive_provide final;

	auto context_function_update(
		ecsact::codegen_plugin_context& ctx,
		const common_vars&              names
	) -> handle_exclusive_provide final;

	auto context_function_has(
		ecsact::codegen_plugin_context& ctx,
		const common_vars&              names
	) -> handle_exclusive_provide final;

	auto context_function_generate(
		ecsact::codegen_plugin_context& ctx,
		const common_vars&              names
	) -> handle_exclusive_provide final;

	auto context_function_parent(
		ecsact::codegen_plugin_context& ctx,
		const common_vars&              names
	) -> handle_exclusive_provide final;

	auto context_function_other(
		ecsact::codegen_plugin_context& ctx,
		const common_vars&              names
	) -> handle_exclusive_provide final;

	[[nodiscard]] virtual auto context_function_stream_toggle(
		ecsact::codegen_plugin_context&                   ctx,
		const ecsact::rt_entt_codegen::core::common_vars& names
	) -> handle_exclusive_provide;

	auto entity_iteration(
		ecsact::codegen_plugin_context&                   ctx,
		ecsact_system_like_id                             sys_like_id,
		const ecsact::rt_entt_codegen::core::common_vars& names,
		std::function<void()>                             iter_func
	) -> handle_exclusive_provide;

	auto system_impl(
		ecsact::codegen_plugin_context& ctx,
		const common_vars&              names
	) -> handle_exclusive_provide final;

	auto provide_context_init(
		ecsact::codegen_plugin_context&                   ctx,
		const ecsact::rt_entt_codegen::core::common_vars& names,
		std::string_view                                  context_type_name
	) -> handle_exclusive_provide;

	auto pre_exec_system_impl_context_init(
		ecsact::codegen_plugin_context&                   ctx,
		const ecsact::rt_entt_codegen::core::common_vars& names,
		std::string_view                                  context_type_name
	) -> void;

private:
	std::unordered_map<ecsact_component_like_id, ecsact_system_capability>
		sys_caps;

	std::string view_type_name;
};
} // namespace ecsact::rt_entt_codegen::core::provider
