/**
 * This file contains _some_ of the Ecsact core module implementations. Other
 * implementations are generated through the ecsact_rt_entt_codegen plugin.
 *
 * Generally speaking if the implementation requires any type information
 * derived from the input Ecsact files they will not be defined here.
 */

#include "ecsact/runtime/core.h"
#include "ecsact/entt/detail/globals.hh"
#include "ecsact/entt/registry_util.hh"
#include "ecsact/entt/entity.hh"

void ecsact_destroy_registry(ecsact_registry_id reg_id) {
	auto& reg = ecsact::entt::get_registry(reg_id);
	reg = {};
	assert(reg.template storage<entt::entity>().in_use() == 0);
}

auto ecsact_create_registry(const char* registry_name) -> ecsact_registry_id {
	auto&& [registry_id, reg] = ecsact::entt::create_registry();
	ecsact::entt::ecsact_init_registry_storage(reg);
	return registry_id;
}

auto ecsact_clone_registry( //
	ecsact_registry_id reg_id,
	const char*        name
) -> ecsact_registry_id {
	auto& reg = ecsact::entt::get_registry(reg_id);
	auto  cloned_reg_id = ecsact_create_registry(name);
	auto& cloned_reg = ecsact::entt::get_registry(cloned_reg_id);
	for(auto&& [entity_id] : reg.template storage<entt::entity>().each()) {
		[[maybe_unused]] auto cloned_entity_id = cloned_reg.create(entity_id);
		assert(cloned_entity_id == entity_id);
	}

	ecsact::entt::copy_components(reg, cloned_reg);

	return cloned_reg_id;
}

auto ecsact_hash_registry(ecsact_registry_id reg_id) -> uint64_t {
	auto& reg = ecsact::entt::get_registry(reg_id);
	return ecsact::entt::hash_registry(reg);
}

void ecsact_clear_registry(ecsact_registry_id reg_id) {
	auto& reg = ecsact::entt::get_registry(reg_id);
	reg = {};
	assert(reg.template storage<entt::entity>().in_use() == 0);
}

ecsact_entity_id ecsact_create_entity(ecsact_registry_id reg_id) {
	auto& reg = ecsact::entt::get_registry(reg_id);
	return ecsact::entt::entity_id{reg.create()};
}

void ecsact_ensure_entity(
	ecsact_registry_id reg_id,
	ecsact_entity_id   entity_id
) {
	auto  entity = ecsact::entt::entity_id{entity_id};
	auto& reg = ecsact::entt::get_registry(reg_id);
	if(!reg.valid(entity)) {
		auto new_entity_id = ecsact::entt::entity_id{reg.create(entity)};
		assert(entity == new_entity_id);
	}
}

bool ecsact_entity_exists(
	ecsact_registry_id reg_id,
	ecsact_entity_id   entity_id
) {
	auto& reg = ecsact::entt::get_registry(reg_id);
	return reg.valid(ecsact::entt::entity_id{entity_id});
}

void ecsact_destroy_entity(
	ecsact_registry_id reg_id,
	ecsact_entity_id   entity_id
) {
	auto& reg = ecsact::entt::get_registry(reg_id);
	reg.destroy(ecsact::entt::entity_id{entity_id});
}

int ecsact_count_entities(ecsact_registry_id reg_id) {
	auto& reg = ecsact::entt::get_registry(reg_id);
	return static_cast<int>(reg.template storage<entt::entity>().in_use());
}

void ecsact_get_entities(
	ecsact_registry_id reg_id,
	int                max_entities_count,
	ecsact_entity_id*  out_entities,
	int*               out_entities_count
) {
	auto& reg = ecsact::entt::get_registry(reg_id);

	int entities_count =
		static_cast<int>(reg.template storage<entt::entity>().in_use());
	max_entities_count = std::min(entities_count, max_entities_count);

	{
		// TODO(zaucy): Using `info.registry.each` is poor when max entities
		// count is less than the amount of entities in the registry.
		// Replace with a different fn such `as info.registry.data`
		int i = 0;

		for(auto&& [entity_id] : reg.template storage<entt::entity>().each()) {
			if(i >= max_entities_count) {
				return;
			}

			out_entities[i] = ecsact::entt::entity_id{entity_id};
			++i;
		}
	}

	if(out_entities_count != nullptr) {
		*out_entities_count = entities_count;
	}
}

ecsact_add_error ecsact_add_component(
	ecsact_registry_id  reg_id,
	ecsact_entity_id    entity_id,
	ecsact_component_id component_id,
	const void*         component_data
) {
	using ecsact::entt::detail::globals::add_component_fns;
	auto fn_itr = add_component_fns.find(component_id);
	assert(fn_itr != add_component_fns.end());
	return fn_itr->second(reg_id, entity_id, component_id, component_data);
}

bool ecsact_has_component(
	ecsact_registry_id  reg_id,
	ecsact_entity_id    entity_id,
	ecsact_component_id component_id,
	const void*         indexed_fields
) {
	using ecsact::entt::detail::globals::has_component_fns;
	auto fn_itr = has_component_fns.find(component_id);
	assert(fn_itr != has_component_fns.end());
	return fn_itr->second(reg_id, entity_id, component_id, indexed_fields);
}

const void* ecsact_get_component(
	ecsact_registry_id  reg_id,
	ecsact_entity_id    entity_id,
	ecsact_component_id component_id,
	const void*         indexed_fields
) {
	using ecsact::entt::detail::globals::get_component_fns;
	auto fn_itr = get_component_fns.find(component_id);
	assert(fn_itr != get_component_fns.end());
	return fn_itr->second(reg_id, entity_id, component_id, indexed_fields);
}

int ecsact_count_components(
	ecsact_registry_id registry_id,
	ecsact_entity_id   entity_id
) {
	using ecsact::entt::detail::globals::all_component_ids;

	int component_count = 0;
	for(auto comp_id : all_component_ids) {
		if(ecsact_has_component(registry_id, entity_id, comp_id, nullptr)) {
			component_count += 1;
		}
	}
	return component_count;
}

void ecsact_each_component(
	ecsact_registry_id             registry_id,
	ecsact_entity_id               entity_id,
	ecsact_each_component_callback callback,
	void*                          callback_user_data
) {
	using ecsact::entt::detail::globals::all_component_ids;

	for(auto comp_id : all_component_ids) {
		if(ecsact_has_component(registry_id, entity_id, comp_id, nullptr)) {
			callback(
				comp_id,
				ecsact_get_component(registry_id, entity_id, comp_id, nullptr),
				callback_user_data
			);
		}
	}
}

void ecsact_get_components(
	ecsact_registry_id   registry_id,
	ecsact_entity_id     entity_id,
	int                  max_components_count,
	ecsact_component_id* out_component_ids,
	const void**         out_components_data,
	int*                 out_components_count
) {
	using ecsact::entt::detail::globals::all_component_ids;

	auto index = 0;
	for(auto comp_id : all_component_ids) {
		if(ecsact_has_component(registry_id, entity_id, comp_id, nullptr)) {
			if(index < max_components_count) {
				out_component_ids[index] = comp_id;
				out_components_data[index] =
					ecsact_get_component(registry_id, entity_id, comp_id, nullptr);
			}
			index += 1;
		}
	}

	if(out_components_count != nullptr) {
		*out_components_count = index;
	}
}

ecsact_update_error ecsact_update_component(
	ecsact_registry_id  reg_id,
	ecsact_entity_id    entity_id,
	ecsact_component_id component_id,
	const void*         component_data,
	const void*         indexed_fields
) {
	using ecsact::entt::detail::globals::update_component_fns;
	auto fn_itr = update_component_fns.find(component_id);
	assert(fn_itr != update_component_fns.end());
	return fn_itr
		->second(reg_id, entity_id, component_id, component_data, indexed_fields);
}

void ecsact_remove_component(
	ecsact_registry_id  reg_id,
	ecsact_entity_id    entity_id,
	ecsact_component_id component_id,
	const void*         indexed_fields
) {
	using ecsact::entt::detail::globals::remove_component_fns;
	auto fn_itr = remove_component_fns.find(component_id);
	assert(fn_itr != remove_component_fns.end());
	return fn_itr->second(reg_id, entity_id, component_id, indexed_fields);
}

ecsact_stream_error ecsact_stream(
	ecsact_registry_id  reg_id,
	ecsact_entity_id    entity_id,
	ecsact_component_id component_id,
	const void*         component_data,
	const void*         indexed_fields
) {
	using ecsact::entt::detail::globals::ecsact_stream_fns;
	auto fn_itr = ecsact_stream_fns.find(component_id);
	assert(fn_itr != ecsact_stream_fns.end());

	return fn_itr->second( //
		reg_id,
		entity_id,
		component_id,
		component_data,
		indexed_fields
	);
}
