#include "parallel.hh"

#include <set>
#include <vector>
#include <type_traits>
#include <format>
#include "ecsact/lang-support/lang-cc.hh"
#include "rt_entt_codegen/shared/system_variant.hh"
#include "system_variant.hh"
#include "ecsact/runtime/meta.hh"

using ecsact::rt_entt_codegen::system_like_id_variant;

static auto loop_iterator(
	const std::vector<system_like_id_variant>&          system_list,
	const std::vector<system_like_id_variant>::iterator begin,
	std::vector<std::vector<system_like_id_variant>>&   parallel_system_cluster
) -> void;

auto ecsact::rt_entt_codegen::parallel::get_parallel_execution_cluster(
	ecsact::codegen_plugin_context&     ctx,
	std::vector<system_like_id_variant> system_list,
	std::string                         parent_context
) -> std::vector<std::vector<system_like_id_variant>> {
	auto parallel_system_cluster =
		std::vector<std::vector<system_like_id_variant>>{};

	loop_iterator(system_list, system_list.begin(), parallel_system_cluster);

	return parallel_system_cluster;
}

static auto is_capability_safe(ecsact_system_capability capability) -> bool {
	std::underlying_type_t<ecsact_system_capability> unsafe_caps =
		ECSACT_SYS_CAP_ADDS | ECSACT_SYS_CAP_REMOVES | ECSACT_SYS_CAP_WRITEONLY |
		ECSACT_SYS_CAP_STREAM_TOGGLE;
	unsafe_caps &= ~(ECSACT_SYS_CAP_EXCLUDE | ECSACT_SYS_CAP_INCLUDE);

	return (unsafe_caps & capability) == 0b0;
}

auto ecsact::rt_entt_codegen::parallel::print_parallel_execution_cluster(
	ecsact::codegen_plugin_context& ctx,
	const std::vector<std::vector<system_like_id_variant>>&
		parallel_system_cluster
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;

	for(const auto& systems_to_parallel : parallel_system_cluster) {
		if(systems_to_parallel.size() == 1) {
			auto single_system_like_variant = systems_to_parallel.begin();

			auto sync_sys_name = cpp_identifier(ecsact::meta::decl_full_name(
				single_system_like_variant->get_sys_like_id()
			));

			if(single_system_like_variant->is_action()) {
				ctx.write(std::format(
					"ecsact::entt::execute_actions<{}>(registry, {}, "
					"actions_map);\n",
					sync_sys_name,
					"nullptr"
				));
			}
			if(single_system_like_variant->is_system()) {
				ctx.write(std::format(
					"ecsact::entt::execute_system<{}>(registry, {}, "
					"actions_map);\n",
					sync_sys_name,
					"nullptr"
				));
			}
			continue;
		}
		if(systems_to_parallel.size() == 0) {
		}

		ctx.write("execute_parallel_cluster(registry, nullptr, ");
		ctx.write(std::format(
			"std::array<exec_entry_t, {}> {{\n",
			systems_to_parallel.size()
		));
		for(const auto system_like_id_variant : systems_to_parallel) {
			auto cpp_decl_name =
				cpp_identifier(ecsact::meta::decl_full_name(system_like_id_variant));

			if(system_like_id_variant.is_action()) {
				ctx.write(
					"\texec_entry_t{&ecsact::entt::execute_actions<",
					cpp_decl_name,
					">, actions_map},\n"
				);
			} else if(system_like_id_variant.is_system()) {
				ctx.write(
					"\texec_entry_t{&ecsact::entt::execute_system<",
					cpp_decl_name,
					">, actions_map},\n"
				);
			} else {
				ctx.write("// ??? unhandled ??? ", cpp_decl_name, "\n");
			}
		}
		ctx.write("});\n");
	}
}

static auto is_capability_safe_entities(
	const system_like_id_variant sys_like_id,
	ecsact_system_capability     capability
) -> bool {
	if(!ecsact::meta::get_system_generates_ids(sys_like_id).empty()) {
		return false;
	}

	std::underlying_type_t<ecsact_system_capability> unsafe_caps =
		ECSACT_SYS_CAP_ADDS | ECSACT_SYS_CAP_REMOVES | ECSACT_SYS_CAP_STREAM_TOGGLE;
	unsafe_caps &= ~(ECSACT_SYS_CAP_EXCLUDE | ECSACT_SYS_CAP_INCLUDE);

	return (unsafe_caps & capability) == 0b0;
}

/*
 * Checks if a parent system's entities can run in parallel with its child
 * systems.
 * Return true if the parent and child entities to have no components in common
 */
static auto can_parent_and_child_system_parallel(
	const std::vector<ecsact_system_id>& child_system_ids,
	const std::unordered_map<ecsact_component_like_id, ecsact_system_capability>&
		capabilities
) -> bool {
	using ecsact::meta::decl_full_name;

	for(const auto child_sys_id : child_system_ids) {
		auto testing_sys_name = decl_full_name(child_sys_id);

		auto child_capabilities = ecsact::meta::system_capabilities(child_sys_id);

		for(const auto& [child_comp_id, child_capability] : child_capabilities) {
			for(const auto& [comp_id, capability] : capabilities) {
				if(comp_id == child_comp_id) {
					if(!is_capability_safe(capability) ||
						 !is_capability_safe(child_capability)) {
						return false;
					}
				}
			}
		}
	}
	return true;
}

auto ecsact::rt_entt_codegen::parallel::can_entities_parallel(
	const system_like_id_variant sys_like_id
) -> bool {
	using ecsact::meta::decl_full_name;

	auto capabilities = ecsact::meta::system_capabilities(sys_like_id);
	auto testing_sys_name = decl_full_name(sys_like_id);
	for(const auto& [comp_id, capability] : capabilities) {
		if(!is_capability_safe_entities(sys_like_id, capability)) {
			return false;
		}

		if(sys_like_id.is_system()) {
			int lazy_iteration_rate =
				ecsact_meta_get_lazy_iteration_rate(sys_like_id.as_system());
			if(lazy_iteration_rate > 0) {
				return false;
			}
		}
	}

	auto child_ids = ecsact::meta::get_child_system_ids(sys_like_id);
	if(!child_ids.empty()) {
		return can_parent_and_child_system_parallel(child_ids, capabilities);
	}
	return true;
}

/**
 * Quick check to see if a system should run independently regardless of it's
 * system capbilities.
 */
static auto should_run_independently(ecsact_system_like_id id) -> bool {
	// User has explicitly marked a system as not parallel; respect that.
	if(ecsact_meta_get_system_parallel_execution(id) == ECSACT_PAR_EXEC_DENY) {
		return true;
	}

	// Generator systems increase storage so they may not run in parallel with
	// other systems.
	if(!ecsact::meta::get_system_generates_ids(id).empty()) {
		return true;
	}

	return false;
}

static auto loop_iterator(
	const std::vector<system_like_id_variant>&          system_list,
	const std::vector<system_like_id_variant>::iterator begin,
	std::vector<std::vector<system_like_id_variant>>&   parallel_system_cluster
) -> void {
	std::vector<system_like_id_variant> parallel_system_list;
	auto unsafe_comps = std::set<ecsact_component_like_id>{};
	auto safe_ro_comps = std::set<ecsact_component_like_id>{};

	using ecsact::meta::decl_full_name;

	for(auto iterator = begin; iterator != system_list.end(); iterator++) {
		auto sys_like_id = *iterator;

		if(should_run_independently(sys_like_id)) {
			if(!parallel_system_list.empty()) {
				parallel_system_cluster.push_back(parallel_system_list);
			}

			parallel_system_cluster.push_back(
				std::vector<system_like_id_variant>{sys_like_id}
			);
			loop_iterator(
				system_list,
				std::next(iterator, 1),
				parallel_system_cluster
			);
			return;
		}

		auto capabilities = ecsact::meta::system_capabilities(sys_like_id);

		auto child_unsafe_comps = std::set<ecsact_component_like_id>{};
		auto child_safe_ro_comps = std::set<ecsact_component_like_id>{};
		auto child_systems = ecsact::meta::get_child_system_ids(sys_like_id);

		for(auto child_sys_id : child_systems) {
			auto cpp_system_name = decl_full_name(child_sys_id);
			auto child_capabilities = ecsact::meta::system_capabilities(child_sys_id);
			for(auto const [child_comp_id, child_capability] : child_capabilities) {
				if(unsafe_comps.contains(child_comp_id)) {
					if(child_capability == ECSACT_SYS_CAP_READONLY ||
						 child_capability == ECSACT_SYS_CAP_OPTIONAL_READONLY) {
						parallel_system_cluster.push_back(parallel_system_list);
						loop_iterator(system_list, iterator, parallel_system_cluster);
						return;
					}
				}

				if(!is_capability_safe(child_capability)) {
					if(unsafe_comps.contains(child_comp_id)) {
						parallel_system_cluster.push_back(parallel_system_list);
						loop_iterator(system_list, iterator, parallel_system_cluster);
						return;
					} else {
						child_unsafe_comps.insert(child_comp_id);
					}
				} else {
					if(child_capability == ECSACT_SYS_CAP_READONLY ||
						 child_capability == ECSACT_SYS_CAP_OPTIONAL_READONLY) {
						child_safe_ro_comps.insert(child_comp_id);
					}
				}
			}
		}

		for(const auto [comp_id, capability] : capabilities) {
			auto cpp_name = decl_full_name(comp_id);

			if(!is_capability_safe(capability)) {
				if(!unsafe_comps.contains(comp_id)) {
					unsafe_comps.insert(comp_id);
				} else {
					parallel_system_cluster.push_back(parallel_system_list);
					loop_iterator(system_list, iterator, parallel_system_cluster);
					return;
				}
			} else {
				if(capability == ECSACT_SYS_CAP_READONLY ||
					 capability == ECSACT_SYS_CAP_OPTIONAL_READONLY) {
					safe_ro_comps.insert(comp_id);
				}
			}

			if(unsafe_comps.contains(comp_id) && safe_ro_comps.contains(comp_id)) {
				parallel_system_cluster.push_back(parallel_system_list);
				loop_iterator(system_list, iterator, parallel_system_cluster);
				return;
			}
		}

		for(auto child_safe_ro_comp : child_safe_ro_comps) {
			if(!safe_ro_comps.contains(child_safe_ro_comp)) {
				safe_ro_comps.insert(child_safe_ro_comp);
			}
		}

		for(auto child_unsafe_comp : child_unsafe_comps) {
			if(!unsafe_comps.contains(child_unsafe_comp)) {
				unsafe_comps.insert(child_unsafe_comp);
			}
		}

		parallel_system_list.push_back(sys_like_id);
	}
	if(!parallel_system_list.empty()) {
		parallel_system_cluster.push_back(parallel_system_list);
	}
}
