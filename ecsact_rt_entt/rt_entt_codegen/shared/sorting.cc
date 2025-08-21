#include "rt_entt_codegen/shared/sorting.hh"

#include "ecsact/runtime/meta.hh"

auto ecsact::rt_entt_codegen::system_needs_sorted_entities( //
	ecsact_system_id id
) -> bool {
	auto needs_sorted_entities = false;

	auto lazy_rate = ecsact_meta_get_lazy_iteration_rate(id);
	if(lazy_rate > 0) {
		needs_sorted_entities = true;
	}

	return needs_sorted_entities;
}

auto ecsact::rt_entt_codegen::get_all_sorted_systems()
	-> std::vector<ecsact_system_like_id> {
	auto sys_like_ids = std::vector<ecsact_system_like_id>{};
	for(auto pkg_id : ecsact::meta::get_package_ids()) {
		auto pkg_sys_ids = ecsact::meta::get_system_ids(pkg_id);

		for(auto sys_id : pkg_sys_ids) {
			if(system_needs_sorted_entities(sys_id)) {
				sys_like_ids.push_back(ecsact_id_cast<ecsact_system_like_id>(sys_id));
			}
		}
	}

	return sys_like_ids;
}
