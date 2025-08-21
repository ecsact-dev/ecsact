#pragma once

#include <functional>
#include <variant>
#include "ecsact/runtime/common.h"

namespace ecsact::interpret::details {

enum class event_ref_id : int;

class event_ref;

/**
 * IDs that can be destroyed callbacks
 */
using destroyable_id_t = std::variant< //
	ecsact_package_id,
	ecsact_component_id,
	ecsact_transient_id,
	ecsact_system_id,
	ecsact_action_id>;

using castable_destroyable_ids_t = std::variant< //
	ecsact_package_id,
	ecsact_component_id,
	ecsact_transient_id,
	ecsact_system_id,
	ecsact_action_id,

	// castables
	ecsact_component_like_id,
	ecsact_composite_id,
	ecsact_system_like_id,
	ecsact_decl_id>;

/**
 * Call @p callback when declaration with @p id is destroyed
 */
auto on_destroy( //
	castable_destroyable_ids_t id,
	std::function<void()>      callback
) -> event_ref;

auto trigger_on_destroy(destroyable_id_t id) -> void;

class event_ref {
	friend auto on_destroy(castable_destroyable_ids_t, std::function<void()>)
		-> event_ref;

	int          destroyable_index_;
	event_ref_id id_;

	event_ref();

public:
	event_ref(event_ref&&);
	~event_ref();
	auto clear() -> void;
};
} // namespace ecsact::interpret::details
