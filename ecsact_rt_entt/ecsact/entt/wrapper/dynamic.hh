#pragma once

#include <cassert>
#include <type_traits>
#include "ecsact/entt/entity.hh"
#include "entt/entity/registry.hpp"
#include "ecsact/entt/registry_util.hh"
#include "ecsact/entt/error_check.hh"
#include "ecsact/entt/detail/internal_markers.hh"
#include "ecsact/entt/event_markers.hh"
#include "ecsact/entt/detail/system_execution_context.hh"

#ifdef TRACY_ENABLE
#	include "tracy/Tracy.hpp"
#endif

namespace ecsact::entt::wrapper::dynamic {

template<typename C>
auto context_add(
	ecsact_system_execution_context*          context,
	[[maybe_unused]] ecsact_component_like_id component_id,
	const void*                               component_data
) -> void {
#ifdef TRACY_ENABLE
	ZoneScopedC(tracy::Color::Teal);
#endif
	using ecsact::entt::component_added;
	using ecsact::entt::component_removed;
	using ecsact::entt::detail::beforeremove_storage;
	using ecsact::entt::detail::exec_beforechange_storage;
	using ecsact::entt::detail::pending_add;

	assert(ecsact_id_cast<ecsact_component_like_id>(C::id) == component_id);

	auto  entity = context->entity;
	auto& registry = *context->registry;

	if constexpr(std::is_empty_v<C>) {
		registry.template emplace_or_replace<pending_add<C>>(entity);
	} else {
		const C* component = static_cast<const C*>(component_data);
		registry.template emplace_or_replace<pending_add<C>>(entity, *component);
		registry.template remove<beforeremove_storage<C>>(entity);
	}

	if constexpr(!C::transient) {
		if(registry.template all_of<component_removed<C>>(entity)) {
			registry.template erase<component_removed<C>>(entity);
		} else {
			registry.template emplace_or_replace<component_added<C>>(entity);
		}
	}
}

template<typename C>
auto component_add_trivial(
	ecsact::entt::registry_t& registry,
	ecsact::entt::entity_id   entity_id
) -> void {
#ifdef TRACY_ENABLE
	ZoneScopedC(tracy::Color::Teal);
#endif
	using ecsact::entt::component_added;
	using ecsact::entt::component_removed;
	using ecsact::entt::detail::pending_add;

	registry.template emplace_or_replace<pending_add<C>>(entity_id);

	if constexpr(!C::transient) {
		if(registry.template all_of<component_removed<C>>(entity_id)) {
			registry.template erase<component_removed<C>>(entity_id);
		} else {
			registry.template emplace_or_replace<component_added<C>>(entity_id);
		}
	}
}

template<typename C>
auto context_remove(
	ecsact_system_execution_context*          context,
	[[maybe_unused]] ecsact_component_like_id component_id,
	const void*                               indexed_field_values,
	auto&                                     view
) -> void {
#ifdef TRACY_ENABLE
	ZoneScopedC(tracy::Color::Orange);
#endif
	assert(ecsact_id_cast<ecsact_component_like_id>(C::id) == component_id);

	using ecsact::entt::component_removed;
	using ecsact::entt::detail::beforeremove_storage;
	using ecsact::entt::detail::pending_remove;

	auto  entity = context->entity;
	auto& registry = *context->registry;

	registry.template remove<component_added<C>>(entity);
	registry.template emplace_or_replace<pending_remove<C>>(entity);
	registry.template emplace_or_replace<component_removed<C>>(entity);

	// Stop here (tag)
	if constexpr(!std::is_empty_v<C>) {
		auto component = view.template get<C>(entity);

		auto& remove_storage =
			registry.template emplace_or_replace<beforeremove_storage<C>>(entity);

		remove_storage.value = component;
	}
}

template<typename C>
auto component_remove_trivial(
	ecsact::entt::registry_t& registry,
	ecsact::entt::entity_id   entity_id,
	auto&                     view
) -> void {
#ifdef TRACY_ENABLE
	ZoneScopedC(tracy::Color::Orange);
#endif
	using ecsact::entt::component_removed;
	using ecsact::entt::detail::beforeremove_storage;
	using ecsact::entt::detail::pending_remove;

	registry.template remove<component_added<C>>(entity_id);
	registry.template emplace_or_replace<pending_remove<C>>(entity_id);
	registry.template emplace_or_replace<component_removed<C>>(entity_id);

	if constexpr(!std::is_empty_v<C>) {
		auto component = view.template get<C>(entity_id);

		auto& remove_storage =
			registry.template emplace_or_replace<beforeremove_storage<C>>(entity_id);

		remove_storage.value = component;
	}
}

template<typename C>
auto context_get(
	ecsact_system_execution_context*          context,
	[[maybe_unused]] ecsact_component_like_id component_id,
	void*                                     out_component_data,
	const void*                               indexed_field_values,
	auto&                                     view
) -> void {
	auto entity = context->entity;

	*static_cast<C*>(out_component_data) = view.template get<C>(entity);
}

template<typename C>
auto context_update(
	ecsact_system_execution_context*          context,
	[[maybe_unused]] ecsact_component_like_id component_id,
	const void*                               in_component_data,
	const void*                               indexed_field_values,
	auto&                                     view
) -> void {
	using ecsact::entt::detail::exec_beforechange_storage;
	// TODO(Kelwan): for remove, beforeremove_storage

	auto entity = context->entity;

	const auto& in_component = *static_cast<const C*>(in_component_data);
	auto& beforechange = view.template get<exec_beforechange_storage<C>>(entity);
	auto& current_component = view.template get<C>(entity);

	if(!beforechange.has_update_occurred) {
		beforechange.value = current_component;
		beforechange.has_update_occurred = true;
	}
	current_component = in_component;
}

template<typename C>
auto context_has(
	ecsact_system_execution_context*          context,
	[[maybe_unused]] ecsact_component_like_id component_id,
	const void*                               indexed_fields
) -> bool {
	static_assert(
		!C::has_assoc_fields,
		"Ecsact RT EnTT doesn't support indexed fields (yet)"
	);

	auto  entity = context->entity;
	auto& registry = *context->registry;

	return registry.template any_of<C>(entity);
}

template<typename C>
auto context_stream_toggle(
	ecsact_system_execution_context*     context,
	[[maybe_unused]] ecsact_component_id component_id,
	bool                                 streaming_enabled,
	const void*                          indexed_fields
) -> void {
	using ecsact::entt::detail::run_on_stream;

	static_assert(
		!C::has_assoc_fields,
		"Ecsact RT EnTT doesn't support indexed fields (yet)"
	);

	auto  entity = context->entity;
	auto& registry = *context->registry;

	if(streaming_enabled) {
		if(registry.any_of<run_on_stream<C>>(entity)) {
			registry.template remove<run_on_stream<C>>(entity);
		}
	} else {
		if(!registry.any_of<run_on_stream<C>>(entity)) {
			registry.template emplace<run_on_stream<C>>(entity);
		}
	}
}

template<typename C>
auto context_generate_add(
	ecsact_system_execution_context* context,
	ecsact_component_id              component_id,
	const void*                      component_data,
	const void*                      indexed_fields,
	ecsact::entt::entity_id          entity
) -> void {
	using ecsact::entt::detail::pending_add;

	static_assert(
		!C::has_assoc_fields,
		"Ecsact RT EnTT doesn't support indexed fields (yet)"
	);

	auto& registry = *context->registry;

	const auto& component = *static_cast<const C*>(component_data);
	registry.template emplace<pending_add<C>>(entity, component);
	registry.template emplace_or_replace<component_added<C>>(entity);
}

} // namespace ecsact::entt::wrapper::dynamic
