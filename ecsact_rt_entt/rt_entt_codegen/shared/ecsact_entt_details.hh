#pragma once

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include "ecsact/runtime/common.h"

namespace ecsact::rt_entt_codegen {

struct other_key {
	ecsact_component_like_id component_like_id;
	ecsact_field_id          field_id;

	auto operator<=>(const other_key& k) const = default;
};

using generate_t =
	std::vector<std::unordered_map<ecsact_component_id, ecsact_system_generate>>;

struct ecsact_entt_system_details {
	/** Components/transients included in EnTT view/group */
	std::unordered_set<ecsact_component_like_id> get_comps;

	/** Components/transients excluded in EnTT view/group */
	std::unordered_set<ecsact_component_like_id> exclude_comps;

	/** Components this system is allowed to read */
	std::unordered_set<ecsact_component_like_id> readable_comps;

	/** Components this system is allowed to write */
	std::unordered_set<ecsact_component_like_id> writable_comps;

	/** Components this system is allowed to add */
	std::unordered_set<ecsact_component_like_id> addable_comps;

	/** Components this system is allowed to remove */
	std::unordered_set<ecsact_component_like_id> removable_comps;

	/** Streaming components on the system */
	std::unordered_set<ecsact_component_like_id> stream_comps;

	/** A map containing this system's generated component ids and its
	 * requirements*/
	generate_t generate_comps;

	static auto from_system_like( //
		ecsact_system_like_id sys_like_id
	) -> ecsact_entt_system_details;

	static auto from_capabilities( //
		std::unordered_map<ecsact_component_like_id, ecsact_system_capability> caps
	) -> ecsact_entt_system_details;

private:
	static auto fill_system_details(
		ecsact_entt_system_details& in_details,
		const std::
			unordered_map<ecsact_component_like_id, ecsact_system_capability>& caps
	) -> void;
};

/**
 * Details about an the main Ecsact package in relation to EnTT runtime. Package
 * dependencies are flattened.
 */
struct ecsact_entt_details {
	static auto from_package(ecsact_package_id pkg_id) -> ecsact_entt_details;

	// Top level sysetms/actions in execution order
	std::vector<ecsact_system_like_id> top_execution_order;

	std::unordered_set<ecsact_system_id> all_systems;

	std::unordered_set<ecsact_action_id> all_actions;

	std::unordered_set<ecsact_component_id> all_components;

	inline auto is_action(ecsact_system_like_id id) const noexcept -> bool {
		return all_actions.contains(static_cast<ecsact_action_id>(id));
	}

	inline auto is_system(ecsact_system_like_id id) const noexcept -> bool {
		return all_systems.contains(static_cast<ecsact_system_id>(id));
	}
};

} // namespace ecsact::rt_entt_codegen
