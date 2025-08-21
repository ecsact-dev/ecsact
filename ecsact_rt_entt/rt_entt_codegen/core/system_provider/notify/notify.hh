#pragma once

#include "rt_entt_codegen/core/system_provider/system_provider.hh"
#include "rt_entt_codegen/core/sys_exec/sys_exec.hh"

namespace ecsact::rt_entt_codegen::core::provider {
class notify final : public system_provider {
public:
	using system_provider::system_provider;

	auto initialization(
		ecsact::codegen_plugin_context& ctx,
		const common_vars&              names
	) -> void {
	}

	auto before_make_view_or_group(
		ecsact::codegen_plugin_context& ctx,
		const common_vars&              names,
		std::vector<std::string>&       additional_view_components
	) -> void final;

private:
	/*
	 * Prints the specialized views for ecsact_system_notify_settings components
	 */
	auto print_system_notify_views(
		ecsact::codegen_plugin_context& ctx,
		ecsact_system_like_id           system_id,
		std::string                     registry_name
	) -> void;
};
} // namespace ecsact::rt_entt_codegen::core::provider
