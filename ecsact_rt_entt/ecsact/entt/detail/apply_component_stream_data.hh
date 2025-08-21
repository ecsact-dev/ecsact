#pragma once

#include <entt/entt.hpp>

#include "ecsact/entt/detail/internal_markers.hh"

namespace ecsact::entt::detail {
template<typename C>
auto apply_component_stream_data(
	::entt::registry& main_reg,
	::entt::registry& stream_reg
) -> void {
	auto view = main_reg.template view<
		C>(::entt::exclude<ecsact::entt::detail::run_on_stream<C>>);

	for(auto entity : view) {
		if(!stream_reg.any_of<C>(entity)) {
			continue;
		}

		auto& in_component = stream_reg.get<C>(entity);
		if(main_reg.any_of<C>(entity)) {
			auto& current_comp = main_reg.get<C>(entity);

			auto& beforechange =
				main_reg.template get<exec_beforechange_storage<C>>(entity);

			if(!beforechange.has_update_occurred) {
				beforechange.value = current_comp;
				beforechange.has_update_occurred = true;
			}
			current_comp = in_component;
		}
	}
}
} // namespace ecsact::entt::detail
