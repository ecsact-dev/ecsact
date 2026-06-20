#ifndef ECSACT_RT_ENTT_RT_ENTT_CODEGEN_SHARED_SORTING_HH_
#define ECSACT_RT_ENTT_RT_ENTT_CODEGEN_SHARED_SORTING_HH_

#include <vector>

#include "ecsact/runtime/common.h"

namespace ecsact::rt_entt_codegen {
auto system_needs_sorted_entities(ecsact_system_id id) -> bool;
auto get_all_sorted_systems() -> std::vector<ecsact_system_like_id>;
} // namespace ecsact::rt_entt_codegen

#endif // ECSACT_RT_ENTT_RT_ENTT_CODEGEN_SHARED_SORTING_HH_
