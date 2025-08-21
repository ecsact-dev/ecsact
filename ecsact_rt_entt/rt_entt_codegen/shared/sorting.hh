#pragma once

#include <vector>

#include "ecsact/runtime/common.h"

namespace ecsact::rt_entt_codegen {
auto system_needs_sorted_entities(ecsact_system_id id) -> bool;
auto get_all_sorted_systems() -> std::vector<ecsact_system_like_id>;
} // namespace ecsact::rt_entt_codegen
