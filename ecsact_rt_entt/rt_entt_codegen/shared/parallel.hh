#pragma once

#include "ecsact/codegen/plugin.hh"
#include "rt_entt_codegen/shared/ecsact_entt_details.hh"
#include "system_variant.hh"

namespace ecsact::rt_entt_codegen::parallel {
auto get_parallel_execution_cluster(
	ecsact::codegen_plugin_context&     ctx,
	std::vector<system_like_id_variant> system_list,
	std::string                         parent_context = "nullptr"
) -> std::vector<std::vector<system_like_id_variant>>;

auto print_parallel_execution_cluster(
	ecsact::codegen_plugin_context& ctx,
	const std::vector<std::vector<system_like_id_variant>>&
		parallel_system_cluster
) -> void;

/*
 * Checks if the entities in a system can run in parallel
 */
auto can_entities_parallel(const system_like_id_variant sys_like_id) -> bool;

} // namespace ecsact::rt_entt_codegen::parallel
