#pragma once

#include <unordered_map>
#include <ranges>
#include <concepts>
#include "ecsact/runtime/common.h"
#include "ecsact/runtime/meta.hh"
#include "rt_entt_codegen/shared/ecsact_entt_details.hh"

namespace ecsact::rt_entt_codegen {
template<typename SystemLikeID>
auto get_all_deep_capabilities( //
	SystemLikeID system_like_id
) -> std::unordered_map<ecsact_component_like_id, ecsact_system_capability> {
	auto capabilities = ecsact::meta::system_capabilities(system_like_id);

	auto all_capabilities =
		std::unordered_map<ecsact_component_like_id, ecsact_system_capability>{};

	for(auto&& [comp_id, capability] : capabilities) {
		all_capabilities[comp_id] = static_cast<ecsact_system_capability>(
			capability | all_capabilities[comp_id]
		);
	}

	return all_capabilities;
}

template<typename SystemLikeID>
inline auto system_comps_with_caps(
	SystemLikeID             system_like_id,
	ecsact_system_capability sys_cap
) -> std::vector<ecsact_component_like_id> {
	auto found_comps = std::vector<ecsact_component_like_id>{};
	auto capabilities = get_all_deep_capabilities(system_like_id);
	for(auto&& [comp_id, capability] : capabilities) {
		if((capability & sys_cap) == sys_cap) {
			found_comps.insert(found_comps.end(), comp_id);
		}
	}

	return found_comps;
}

inline auto comps_with_caps(
	const ecsact_entt_details& details,
	ecsact_system_capability   sys_cap
) -> std::vector<ecsact_component_like_id> {
	auto found_comps = std::vector<ecsact_component_like_id>{};

	for(auto system_id : details.all_systems) {
		auto capabilities = get_all_deep_capabilities(system_id);
		for(auto&& [comp_id, capability] : capabilities) {
			if((capability & sys_cap) == sys_cap) {
				found_comps.insert(found_comps.end(), comp_id);
			}
		}
	}

	for(auto action_id : details.all_actions) {
		auto capabilities = get_all_deep_capabilities(action_id);
		for(auto&& [comp_id, capability] : capabilities) {
			if((capability & sys_cap) == sys_cap) {
				found_comps.insert(found_comps.end(), comp_id);
			}
		}
	}

	std::ranges::sort(found_comps);
	auto unique_found_comps = std::vector<ecsact_component_like_id>{};
	unique_found_comps.reserve(found_comps.size());
	std::ranges::copy(found_comps, std::back_inserter(unique_found_comps));

	return unique_found_comps;
}

} // namespace ecsact::rt_entt_codegen
