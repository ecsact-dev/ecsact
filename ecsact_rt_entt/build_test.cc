// Include all headers for the sake of a quick build test
#include "entt/entt.hpp" // IWYU pragma: keep
#include "ecsact/entt/detail/apply_pending.hh" // IWYU pragma: keep
#include "ecsact/entt/detail/bytes.hh" // IWYU pragma: keep
#include "ecsact/entt/detail/execution_events_collector.hh" // IWYU pragma: keep
#include "ecsact/entt/detail/globals.hh" // IWYU pragma: keep
#include "ecsact/entt/detail/hash.hh" // IWYU pragma: keep
#include "ecsact/entt/detail/internal_markers.hh" // IWYU pragma: keep
#include "ecsact/entt/detail/registry.hh" // IWYU pragma: keep
#include "ecsact/entt/detail/system_execution_context.hh" // IWYU pragma: keep
#include "ecsact/entt/detail/apply_component_stream_data.hh" // IWYU pragma: keep
#include "ecsact/entt/entity.hh" // IWYU pragma: keep
#include "ecsact/entt/error_check.hh" // IWYU pragma: keep
#include "ecsact/entt/event_markers.hh" // IWYU pragma: keep
#include "ecsact/entt/execution.hh" // IWYU pragma: keep
#include "ecsact/entt/registry_util.hh" // IWYU pragma: keep
#include "ecsact/entt/wrapper/core.hh" // IWYU pragma: keep
#include "ecsact/entt/wrapper/dynamic.hh" // IWYU pragma: keep
#include "ecsact/entt/stream_registries.hh" // IWYU pragma: keep
#include "ecsact/runtime/common.h" // IWYU pragma: keep
#include "entt/entity/registry.hpp" // IWYU pragma: keep

// default assign some global vars for the sake of compiling only
#define MOCK_DEF_GLOBAL(Name)                  \
	decltype(ecsact::entt::detail::globals::Name \
	) ecsact::entt::detail::globals::Name = {}
MOCK_DEF_GLOBAL(registries);
MOCK_DEF_GLOBAL(last_registry_id);
MOCK_DEF_GLOBAL(system_impls);
MOCK_DEF_GLOBAL(all_component_ids);
MOCK_DEF_GLOBAL(add_component_fns);
MOCK_DEF_GLOBAL(get_component_fns);
MOCK_DEF_GLOBAL(has_component_fns);
MOCK_DEF_GLOBAL(update_component_fns);
MOCK_DEF_GLOBAL(remove_component_fns);
MOCK_DEF_GLOBAL(exec_ctx_action_fns);
MOCK_DEF_GLOBAL(ecsact_stream_fns);

auto main() -> int {
	// This is only here to get compile commands working
	return 0;
}
