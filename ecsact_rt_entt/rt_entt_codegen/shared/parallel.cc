#include "parallel.hh"

#include <set>
#include <vector>
#include <type_traits>
#include <format>
#include "ecsact/lang-support/lang-cc.hh"
#include "rt_entt_codegen/shared/system_variant.hh"
#include "ecsact/runtime/meta.hh"

using ecsact::rt_entt_codegen::system_like_id_variant;

auto ecsact::rt_entt_codegen::parallel::get_system_parallel_execution_cluster(
	ecsact_system_like_id system_id
) -> std::vector<std::vector<system_like_id_variant>> {
	auto parallel_system_cluster =
		std::vector<std::vector<system_like_id_variant>>{};

	auto batch_count = ecsact_meta_count_system_execution_batches(system_id);

	for(int32_t i = 0; batch_count > i; ++i) {
		auto&   batch = parallel_system_cluster.emplace_back();
		int32_t systems_count = 0;
		ecsact_meta_get_system_execution_batch(
			system_id,
			i,
			0,
			nullptr,
			&systems_count
		);
		std::vector<ecsact_system_like_id> batch_systems(systems_count);
		ecsact_meta_get_system_execution_batch(
			system_id,
			i,
			systems_count,
			batch_systems.data(),
			nullptr
		);

		for(auto sys_id : batch_systems) {
			if(ecsact_meta_is_system(sys_id)) {
				batch.push_back(static_cast<ecsact_system_id>(sys_id));
			} else if(ecsact_meta_is_action(sys_id)) {
				batch.push_back(static_cast<ecsact_action_id>(sys_id));
			}
		}
	}

	return parallel_system_cluster;
}

auto ecsact::rt_entt_codegen::parallel::get_parallel_execution_cluster(
	ecsact::codegen_plugin_context& ctx,
	std::string                     parent_context
) -> std::vector<std::vector<system_like_id_variant>> {
	auto parallel_system_cluster =
		std::vector<std::vector<system_like_id_variant>>{};

	auto package_id = ctx.package_id;
	auto batch_count = ecsact_meta_count_execution_batches(package_id);

	for(int32_t i = 0; batch_count > i; ++i) {
		auto&   batch = parallel_system_cluster.emplace_back();
		int32_t systems_count = 0;
		ecsact_meta_get_execution_batch(package_id, i, 0, nullptr, &systems_count);
		std::vector<ecsact_system_like_id> batch_systems(systems_count);
		ecsact_meta_get_execution_batch(
			package_id,
			i,
			systems_count,
			batch_systems.data(),
			nullptr
		);

		for(auto sys_id : batch_systems) {
			if(ecsact_meta_is_system(sys_id)) {
				batch.push_back(static_cast<ecsact_system_id>(sys_id));
			} else if(ecsact_meta_is_action(sys_id)) {
				batch.push_back(static_cast<ecsact_action_id>(sys_id));
			}
		}
	}

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

			auto sync_sys_name = cpp_identifier(
				ecsact::meta::decl_full_name(
					single_system_like_variant->get_sys_like_id()
				)
			);

			if(single_system_like_variant->is_action()) {
				ctx.write(
					std::format(
						"ecsact::entt::execute_actions<{}>(registry, {}, "
						"actions_map);\n",
						sync_sys_name,
						"nullptr"
					)
				);
			}
			if(single_system_like_variant->is_system()) {
				ctx.write(
					std::format(
						"ecsact::entt::execute_system<{}>(registry, {}, "
						"actions_map);\n",
						sync_sys_name,
						"nullptr"
					)
				);
			}
			continue;
		}
		if(systems_to_parallel.size() == 0) {
		}

		ctx.write("execute_parallel_cluster(registry, nullptr, ");
		ctx.write(
			std::format(
				"std::array<exec_entry_t, {}> {{\n",
				systems_to_parallel.size()
			)
		);
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
					if(
						!is_capability_safe(capability) ||
						!is_capability_safe(child_capability)
					) {
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
