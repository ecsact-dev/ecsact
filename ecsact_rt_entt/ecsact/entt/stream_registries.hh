#pragma once

#include <thread>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <ranges>
#include <memory>
#include <entt/entt.hpp>
#include "ecsact/runtime/common.h"
#include "ecsact/entt/entity.hh"

namespace ecsact::entt::stream::detail {
using child_reg_thread_map =
	std::unordered_map<std::thread::id, std::unique_ptr<::entt::registry>>;
using reg_thread_map =
	std::unordered_map<ecsact_registry_id, child_reg_thread_map>;
} // namespace ecsact::entt::stream::detail

namespace ecsact::entt::stream {
class stream_registries {
private:
	detail::reg_thread_map registry_stream_threads;
	std::shared_mutex      stream_mutex;

	template<typename C>
	auto ensure_and_add_entity_and_component(
		std::unique_ptr<::entt::registry>& registry,
		::ecsact::entt::entity_id          entity,
		const C&                           component
	) -> void {
		if(!registry->valid(entity)) {
			auto new_entity = registry->create(entity);
			assert(new_entity == entity.as_entt());
		}

		registry->template emplace_or_replace<C>(entity, component);
	}

public:
	stream_registries() = default;

	template<typename C>
	auto handle_stream(
		ecsact_registry_id registry_id,
		ecsact_entity_id   entity_id,
		const C&           component
	) -> void {
		// Add to map if new threads/registries are introduced

		auto thread_id = std::this_thread::get_id();
		auto entity = ::ecsact::entt::entity_id(entity_id);

		std::shared_lock shared_lk(stream_mutex);
		std::unique_lock lk(stream_mutex, std::defer_lock);

		auto reg_threads_itr = registry_stream_threads.find(registry_id);
		assert(reg_threads_itr != registry_stream_threads.end());

		auto& reg_threads = reg_threads_itr->second;
		auto  reg_thread_itr = reg_threads.find(thread_id);

		if(reg_thread_itr == reg_threads.end()) {
			auto registry = std::make_unique<::entt::registry>();
			ensure_and_add_entity_and_component(registry, entity, component);
			shared_lk.unlock();
			lk.lock();
			reg_threads.insert(
				reg_threads.end(),
				std::pair(thread_id, std::move(registry))
			);
			lk.unlock();
		} else {
			auto& registry = reg_thread_itr->second;
			ensure_and_add_entity_and_component(registry, entity, component);
		}
	}

	auto get_stream_registries()
		-> std::vector<std::unique_ptr<::entt::registry>>;
	auto add_registry(ecsact_registry_id) -> void;
};
} // namespace ecsact::entt::stream
