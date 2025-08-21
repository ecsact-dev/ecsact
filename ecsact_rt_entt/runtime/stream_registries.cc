#include <memory>

#include <entt/entt.hpp>

#include "ecsact/entt/stream_registries.hh"
#include "ecsact/entt/detail/globals.hh"

ecsact::entt::stream::stream_registries
	ecsact::entt::detail::globals::stream_registries = {};

auto ecsact::entt::stream::stream_registries::get_stream_registries()
	-> std::vector<std::unique_ptr<::entt::registry>> {
	auto registries = std::vector<std::unique_ptr<::entt::registry>>{};

	std::unique_lock lk(stream_mutex);
	for(auto&& [reg_id, map] : registry_stream_threads) {
		for(auto&& [thread_id, registry] : map) {
			if(!registry->empty()) {
				registries.push_back(std::move(registry));
				registry = std::make_unique<::entt::registry>();
			} else {
				// TODO(Kelwan): Clean up hanging threads here?
			}
		}
	}
	return registries;
}

auto ecsact::entt::stream::stream_registries::add_registry(
	ecsact_registry_id registry_id
) -> void {
	auto thread_map = detail::child_reg_thread_map{};

	// thread_map.emplace_hint(
	// 	thread_map.end(),
	// 	std::pair(thread_id, std::make_unique<::entt::registry>())
	// );

	std::unique_lock lk(stream_mutex);
	registry_stream_threads.insert(
		registry_stream_threads.end(),
		std::pair(registry_id, std::move(thread_map))
	);
}
