#pragma once

#include <cassert>
#include "entt/entity/registry.hpp"
#include "ecsact/entt/detail/globals.hh"
#include "ecsact/entt/detail/registry.hh"

namespace ecsact::entt {

inline auto get_registry( //
	ecsact_registry_id id
) -> ecsact::entt::registry_t& {
	using ecsact::entt::detail::globals::registries;

	// Check to make sure we're not trying to get a registry that doesn't exist
	// or has been destroyed.
	assert(registries.contains(id));
	return registries.at(id);
}

inline auto create_registry()
	-> std::tuple<ecsact_registry_id, ecsact::entt::registry_t&> {
	using ecsact::entt::detail::globals::last_registry_id;
	using ecsact::entt::detail::globals::registries;

	auto registry_id = static_cast<ecsact_registry_id>(
		++reinterpret_cast<int32_t&>(last_registry_id)
	);
	auto& registry = registries[registry_id];

	ecsact::entt::detail::globals::stream_registries.add_registry(registry_id);

	return {registry_id, std::ref(registry)};
}

auto ecsact_init_registry_storage(::entt::registry& registry) -> void;

auto copy_components( //
	const ::entt::registry& src,
	::entt::registry&       dst
) -> void;

auto hash_registry(const ::entt::registry& reg) -> std::uint64_t;

} // namespace ecsact::entt
