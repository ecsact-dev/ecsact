#include "ecsact_entt_details.hh"

#include <cassert>
#include "ecsact/codegen/plugin.h"
#include "ecsact/codegen/plugin.hh"
#include "ecsact/lang-support/lang-cc.hh"
#include "ecsact/runtime/meta.hh"
#include "rt_entt_codegen/shared/ecsact_entt_details.hh"

using ecsact::rt_entt_codegen::ecsact_entt_details;
using ecsact::rt_entt_codegen::ecsact_entt_system_details;

static auto collect_top_level_systems( //
	ecsact_package_id    pkg_id,
	ecsact_entt_details& details
) -> void {
	auto deps = ecsact::meta::get_dependencies(pkg_id);
	for(auto dep : deps) {
		auto tl_sys_ids = ecsact::meta::get_top_level_systems(dep);
		details.top_execution_order.insert(
			details.top_execution_order.end(),
			tl_sys_ids.begin(),
			tl_sys_ids.end()
		);
	}

	auto main_tl_sys_ids = ecsact::meta::get_top_level_systems(pkg_id);
	details.top_execution_order.insert(
		details.top_execution_order.end(),
		main_tl_sys_ids.begin(),
		main_tl_sys_ids.end()
	);
}

static auto collect_all_systems( //
	ecsact_package_id    pkg_id,
	ecsact_entt_details& details
) -> void {
	auto deps = ecsact::meta::get_dependencies(pkg_id);
	for(auto dep : deps) {
		for(auto id : ecsact::meta::get_system_ids(dep)) {
			details.all_systems.insert(id);
		}
	}

	for(auto id : ecsact::meta::get_system_ids(pkg_id)) {
		details.all_systems.insert(id);
	}
}

static auto collect_all_components( //
	ecsact_package_id    pkg_id,
	ecsact_entt_details& details
) -> void {
	auto deps = ecsact::meta::get_dependencies(pkg_id);
	for(auto dep : deps) {
		for(auto id : ecsact::meta::get_component_ids(dep)) {
			details.all_components.insert(id);
		}
	}

	for(auto id : ecsact::meta::get_component_ids(pkg_id)) {
		details.all_components.insert(id);
	}
}

static auto collect_all_actions( //
	ecsact_package_id    pkg_id,
	ecsact_entt_details& details
) -> void {
	auto deps = ecsact::meta::get_dependencies(pkg_id);
	for(auto dep : deps) {
		for(auto id : ecsact::meta::get_action_ids(dep)) {
			details.all_actions.insert(id);
		}
	}

	for(auto id : ecsact::meta::get_action_ids(pkg_id)) {
		details.all_actions.insert(id);
	}
}

auto ecsact_entt_system_details::fill_system_details(
	ecsact_entt_system_details& out_details,
	const std::unordered_map<ecsact_component_like_id, ecsact_system_capability>&
		caps
) -> void {
	for(auto&& [comp_id, cap] : caps) {
		switch(cap) {
			case ECSACT_SYS_CAP_INCLUDE:
				out_details.get_comps.insert(comp_id);
				break;
			case ECSACT_SYS_CAP_READONLY:
				out_details.get_comps.insert(comp_id);
				out_details.readable_comps.insert(comp_id);
				break;
			case ECSACT_SYS_CAP_WRITEONLY:
				out_details.get_comps.insert(comp_id);
				out_details.writable_comps.insert(comp_id);
				break;
			case ECSACT_SYS_CAP_READWRITE:
				out_details.get_comps.insert(comp_id);
				out_details.readable_comps.insert(comp_id);
				out_details.writable_comps.insert(comp_id);
				break;
			case ECSACT_SYS_CAP_OPTIONAL:
			case ECSACT_SYS_CAP_OPTIONAL_READONLY:
			case ECSACT_SYS_CAP_OPTIONAL_WRITEONLY:
			case ECSACT_SYS_CAP_OPTIONAL_READWRITE:
				assert(false && "optional unimplemented");
				break;
			case ECSACT_SYS_CAP_REMOVES:
				out_details.get_comps.insert(comp_id);
				out_details.removable_comps.insert(comp_id);
				break;
			case ECSACT_SYS_CAP_EXCLUDE:
				out_details.exclude_comps.insert(comp_id);
				break;
			case ECSACT_SYS_CAP_ADDS:
				out_details.exclude_comps.insert(comp_id);
				out_details.addable_comps.insert(comp_id);
				break;
			case ECSACT_SYS_CAP_STREAM_TOGGLE:
				out_details.get_comps.insert(comp_id);
				out_details.stream_comps.insert(comp_id);
		}
	}

	for([[maybe_unused]] auto&& [comp_id, _] : caps) {
		// Sanity check to make sure we've not missing any system comp IDs
		assert(
			out_details.get_comps.contains(comp_id) ||
			out_details.exclude_comps.contains(comp_id)
		);
	}
}

auto ecsact_entt_system_details::from_system_like( //
	ecsact_system_like_id sys_like_id
) -> ecsact_entt_system_details {
	auto details = ecsact_entt_system_details{};
	auto caps = ecsact::meta::system_capabilities(sys_like_id);

	fill_system_details(details, caps);

	auto generate_ids = ecsact::meta::get_system_generates_ids(sys_like_id);
	for(auto gen_id : generate_ids) {
		auto gen_id_map =
			ecsact::meta::get_system_generates_components(sys_like_id, gen_id);
		details.generate_comps.emplace(details.generate_comps.end(), gen_id_map);
	}

	return details;
}

auto ecsact_entt_system_details::from_capabilities( //
	std::unordered_map<ecsact_component_like_id, ecsact_system_capability> caps
) -> ecsact_entt_system_details {
	// NOTE(Kelwan): This does not add generate behaviour or associations into
	// details
	auto details = ecsact_entt_system_details{};

	fill_system_details(details, caps);

	return details;
}

auto ecsact_entt_details::from_package( //
	ecsact_package_id pkg_id
) -> ecsact_entt_details {
	auto details = ecsact_entt_details{};

	collect_top_level_systems(pkg_id, details);
	collect_all_systems(pkg_id, details);
	collect_all_actions(pkg_id, details);
	collect_all_components(pkg_id, details);

	return details;
}
