/**
 * This file contains _some_ of the Ecsact dynamic module implementations. Other
 * implementations are generated through the ecsact_rt_entt_codegen plugin.
 *
 * Generally speaking if the implementation requires any type information
 * derived from the input Ecsact files they will not be defined here.
 */

#include "ecsact/runtime/core.h"
#include "ecsact/entt/detail/globals.hh"
#include "ecsact/entt/registry_util.hh"
#include "ecsact/entt/entity.hh"
#include "ecsact/entt/detail/system_execution_context.hh"

bool ecsact_system_execution_context_same(
	const ecsact_system_execution_context* a,
	const ecsact_system_execution_context* b
) {
	assert(a != nullptr);
	assert(b != nullptr);
	return a->entity == b->entity;
}

ecsact_entity_id ecsact_system_execution_context_entity(
	const ecsact_system_execution_context* context
) {
	assert(context != nullptr);
	return context->entity;
}

// ecsact build currently doesn't have a way to select certain features of
// runtime. So if we're building with 'ecsact build' include all features until
// configuration is possible.
#if defined(ECSACT_ENTT_RUNTIME_DYNAMIC_SYSTEM_IMPLS) || defined(ECSACT_BUILD)
bool ecsact_set_system_execution_impl(
	ecsact_system_like_id        system_id,
	ecsact_system_execution_impl system_exec_impl
) {
	using ecsact::entt::detail::globals::system_impls;
	system_impls[system_id] = system_exec_impl;
	return true;
}
#endif

void ecsact_system_execution_context_generate(
	ecsact_system_execution_context* context,
	int                              component_count,
	ecsact_component_id*             component_ids,
	const void**                     components_data
) {
	assert(context != nullptr);
	return context->generate(component_count, component_ids, components_data);
}

ecsact_system_like_id ecsact_system_execution_context_id(
	ecsact_system_execution_context* context
) {
	return context->id;
}

ecsact_system_execution_context* ecsact_system_execution_context_other(
	ecsact_system_execution_context* context,
	ecsact_system_assoc_id           assoc_id
) {
	assert(context != nullptr);
	return context->other(assoc_id);
}

void ecsact_system_execution_context_add(
	ecsact_system_execution_context* context,
	ecsact_component_like_id         comp_id,
	const void*                      component_data
) {
	assert(context != nullptr);
	return context->add(comp_id, component_data);
}

void ecsact_system_execution_context_remove(
	ecsact_system_execution_context* context,
	ecsact_component_like_id         comp_id,
	const void*                      indexed_fields
) {
	assert(context != nullptr);
	return context->remove(comp_id, indexed_fields);
}

void ecsact_system_execution_context_get(
	ecsact_system_execution_context* context,
	ecsact_component_like_id         comp_id,
	void*                            out_component_data,
	const void*                      indexed_fields
) {
	assert(context != nullptr);
	return context->get(comp_id, out_component_data, indexed_fields);
}

void ecsact_system_execution_context_update(
	ecsact_system_execution_context* context,
	ecsact_component_like_id         comp_id,
	const void*                      component_data,
	const void*                      indexed_fields
) {
	assert(context != nullptr);
	return context->update(comp_id, component_data, indexed_fields);
}

bool ecsact_system_execution_context_has(
	ecsact_system_execution_context* context,
	ecsact_component_like_id         comp_id,
	const void*                      indexed_fields
) {
	assert(context != nullptr);
	return context->has(comp_id, indexed_fields);
}

void ecsact_system_execution_context_action(
	ecsact_system_execution_context* context,
	void*                            out_action_data
) {
	assert(context != nullptr);
	return context->action(out_action_data);
}

void ecsact_system_execution_context_stream_toggle(
	ecsact_system_execution_context* context,
	ecsact_component_id              comp_id,
	bool                             streaming_enabled,
	const void*                      indexed_fields
) {
	assert(context != nullptr);
	return context->stream_toggle(comp_id, streaming_enabled, indexed_fields);
}

const ecsact_system_execution_context* ecsact_system_execution_context_parent(
	ecsact_system_execution_context* context
) {
	assert(context != nullptr);
	return context->parent_ctx;
}
