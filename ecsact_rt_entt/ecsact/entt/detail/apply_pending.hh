#pragma once

#include <entt/entt.hpp>
#include "ecsact/entt/wrapper/core.hh"
#include "ecsact/entt/detail/internal_markers.hh"

namespace ecsact::entt::detail {

template<typename C>
auto apply_pending_add(ecsact::entt::registry_t& registry) -> void {
	if constexpr(std::is_empty_v<C>) {
		registry.view<pending_add<C>>().each([&](auto entity) {
			registry.emplace<C>(entity);
		});
	} else {
		registry.view<pending_add<C>>().each( //
			[&](auto entity, const pending_add<C>& comp) {
				registry.emplace<C>(entity, comp.value);
				registry
					.emplace<exec_beforechange_storage<C>>(entity, comp.value, false);
				add_system_markers_if_needed<C>(registry, entity);
			}
		);
	}
	registry.clear<pending_add<C>>();
}

template<typename C>
auto apply_pending_remove(ecsact::entt::registry_t& registry) -> void {
	registry.view<pending_remove<C>>().each([&](auto entity) {
		if constexpr(!std::is_empty_v<C>) {
			registry.erase<exec_beforechange_storage<C>>(entity);
		}
		registry.erase<C>(entity);
	});
	registry.clear<pending_remove<C>>();
}

} // namespace ecsact::entt::detail
