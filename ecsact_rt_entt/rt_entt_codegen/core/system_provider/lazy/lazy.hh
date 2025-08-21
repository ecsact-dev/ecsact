#pragma once

#include <cstdint>
#include <string>
#include "rt_entt_codegen/core/sys_exec/sys_exec.hh"
#include "rt_entt_codegen/core/system_provider/system_provider.hh"

namespace ecsact::rt_entt_codegen::core::provider {

class lazy final : public system_provider {
public:
	using system_provider::system_provider;

	auto initialization(
		ecsact::codegen_plugin_context& ctx,
		const common_vars&              names
	) -> void final;

	auto before_make_view_or_group(
		ecsact::codegen_plugin_context& ctx,
		const common_vars&              names,
		std::vector<std::string>&       additional_view_components
	) -> void final;

	auto after_make_view_or_group(
		ecsact::codegen_plugin_context& ctx,
		const common_vars&              names
	) -> void final;

	auto pre_exec_system_impl(
		ecsact::codegen_plugin_context& ctx,
		const common_vars&              names
	) -> void final;

	auto post_iteration(
		ecsact::codegen_plugin_context& ctx,
		const common_vars&              names
	) -> void final;

private:
	std::string exec_start_label_name;
	std::string pending_lazy_exec_struct;
	std::string system_sorting_struct_name;

	int32_t lazy_iteration_rate;
};
} // namespace ecsact::rt_entt_codegen::core::provider
