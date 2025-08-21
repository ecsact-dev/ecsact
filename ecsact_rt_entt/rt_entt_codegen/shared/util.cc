#include "rt_entt_codegen/shared/util.hh"

#include <unordered_set>
#include <algorithm>
#include "ecsact/runtime/meta.hh"

static auto collect_pkg_deps(
	ecsact_package_id                      pkg_id,
	std::unordered_set<ecsact_package_id>& out_set
) -> void {
	auto deps = ecsact::meta::get_dependencies(pkg_id);
	for(auto dep : deps) {
		if(out_set.contains(dep)) {
			continue;
		}
		out_set.insert(dep);
		collect_pkg_deps(dep, out_set);
	}
}

static auto all_pkg_ids() -> std::unordered_set<ecsact_package_id> {
	auto ids = std::unordered_set<ecsact_package_id>{};
	auto main_pkg_id = ecsact::meta::main_package();
	ids.insert(*main_pkg_id);
	collect_pkg_deps(*main_pkg_id, ids);
	return ids;
}

template<typename ID>
constexpr auto default_id_min() -> ID {
	return static_cast<ID>(std::numeric_limits<std::underlying_type_t<ID>>::max()
	);
}

template<typename ID>
constexpr auto default_id_max() -> ID {
	return static_cast<ID>(std::numeric_limits<std::underlying_type_t<ID>>::min()
	);
}

template<>
auto ecsact::rt_entt_codegen::util::ecsact_id_min_max<ecsact_component_id>()
	-> std::tuple<ecsact_component_id, ecsact_component_id> {
	auto min = default_id_min<ecsact_component_id>();
	auto max = default_id_max<ecsact_component_id>();
	for(auto pkg_id : all_pkg_ids()) {
		auto comp_ids = ecsact::meta::get_component_ids(pkg_id);
		for(auto comp_id : comp_ids) {
			if(static_cast<int>(comp_id) < static_cast<int>(min)) {
				min = comp_id;
			}

			if(static_cast<int>(comp_id) > static_cast<int>(max)) {
				max = comp_id;
			}
		}
	}

	if(min == default_id_min<ecsact_component_id>()) {
		return {};
	}

	return {min, max};
}

template<>
auto ecsact::rt_entt_codegen::util::ecsact_id_min_max<ecsact_transient_id>()
	-> std::tuple<ecsact_transient_id, ecsact_transient_id> {
	auto min = default_id_min<ecsact_transient_id>();
	auto max = default_id_max<ecsact_transient_id>();
	for(auto pkg_id : all_pkg_ids()) {
		auto trans_ids = ecsact::meta::get_transient_ids(pkg_id);
		for(auto trans_id : trans_ids) {
			if(static_cast<int>(trans_id) < static_cast<int>(min)) {
				min = trans_id;
			}

			if(static_cast<int>(trans_id) > static_cast<int>(max)) {
				max = trans_id;
			}
		}
	}

	if(min == default_id_min<ecsact_transient_id>()) {
		return {};
	}

	return {min, max};
}

template<>
auto ecsact::rt_entt_codegen::util::ecsact_id_min_max<ecsact_component_like_id>(
) -> std::tuple<ecsact_component_like_id, ecsact_component_like_id> {
	auto [transient_min, transient_max] =
		ecsact_id_min_max<ecsact_transient_id>();
	auto [component_min, component_max] =
		ecsact_id_min_max<ecsact_component_id>();

	return {
		static_cast<ecsact_component_like_id>(
			std::min(static_cast<int>(transient_min), static_cast<int>(component_min))
		),
		static_cast<ecsact_component_like_id>(
			std::max(static_cast<int>(transient_max), static_cast<int>(component_max))
		)
	};
}
