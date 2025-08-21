#pragma once

#include <type_traits>

namespace ecsact::entt {

/**
 * Marker to indicate that a component has been added during execution
 */
template<typename C>
struct component_added {};

/**
 * Marker to indicate that a component has been removed
 */
template<typename C>
struct component_removed {};

} // namespace ecsact::entt
