#pragma once

#include "rt_entt_codegen/core/system_provider/system_provider.hh"
#include "rt_entt_codegen/core/sys_exec/sys_exec.hh"

namespace ecsact::rt_entt_codegen::core::provider {
class parallel final : public system_provider {
public:
	using system_provider::system_provider;

	auto entity_iteration(
		ecsact::codegen_plugin_context&                   ctx,
		ecsact_system_like_id                             sys_like_id,
		const ecsact::rt_entt_codegen::core::common_vars& names,
		std::function<void()>                             iter_func
	) -> handle_exclusive_provide;

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
};
} // namespace ecsact::rt_entt_codegen::core::provider
