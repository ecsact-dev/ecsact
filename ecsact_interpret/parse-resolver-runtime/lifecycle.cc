#include "parse-resolver-runtime/lifecycle.hh"

#include <array>
#include "ecsact/runtime/common.h"

using ecsact::interpret::details::castable_destroyable_ids_t;
using ecsact::interpret::details::destroyable_id_t;
using ecsact::interpret::details::event_ref_id;

struct lifecycle_callback_info {
	event_ref_id                          last_event_ref_id = {};
	std::unordered_map<event_ref_id, int> callback_id;
	std::unordered_map<event_ref_id, std::function<void()>> callbacks;

	auto gen_next_id() -> event_ref_id {
		using storage_t = std::underlying_type_t<event_ref_id>;
		reinterpret_cast<storage_t&>(last_event_ref_id) += 1;
		return last_event_ref_id;
	}
};

static std::array< //
	lifecycle_callback_info,
	std::variant_size_v<castable_destroyable_ids_t>>
	lifecycle_info = {};

static auto destroyable_id_as_int(castable_destroyable_ids_t id) -> int {
	return std::visit([](auto id) { return static_cast<int>(id); }, id);
}

static auto destroyable_id_as_int(destroyable_id_t id) -> int {
	return std::visit([](auto id) { return static_cast<int>(id); }, id);
}

static auto callback_indices(destroyable_id_t id) -> std::vector<int> {
	auto result = std::vector<int>{};
	result.emplace_back(static_cast<int>(id.index()));

	auto add_castable_id = [&]<typename T>(T v) -> void {
		result.emplace_back( //
			static_cast<int>(castable_destroyable_ids_t{v}.index())
		);
	};

	std::visit(
		[&]<typename T>(T id) {
			if constexpr(ecsact_id_castable<T, ecsact_component_like_id>()) {
				add_castable_id(ecsact_component_like_id{});
			}
			if constexpr(ecsact_id_castable<T, ecsact_composite_id>()) {
				add_castable_id(ecsact_composite_id{});
			}
			if constexpr(ecsact_id_castable<T, ecsact_system_like_id>()) {
				add_castable_id(ecsact_system_like_id{});
			}
			if constexpr(ecsact_id_castable<T, ecsact_decl_id>()) {
				add_castable_id(ecsact_decl_id{});
			}
		},
		id
	);

	return result;
}

ecsact::interpret::details::event_ref::event_ref() = default;

ecsact::interpret::details::event_ref::event_ref(event_ref&& other) {
	id_ = other.id_;
	other.id_ = {};
}

ecsact::interpret::details::event_ref::~event_ref() {
	clear();
}

auto ecsact::interpret::details::event_ref::clear() -> void {
	if(id_ == event_ref_id{}) {
		return;
	}

	if(destroyable_index_ >= lifecycle_info.size()) {
		return;
	}

	auto& info = lifecycle_info[destroyable_index_];
	// if(info.callbacks.contains(id_)) {
	// 	info.callbacks.erase(id_);
	// }
	// if(info.callback_id.contains(id_)) {
	// 	info.callback_id.erase(id_);
	// }
	id_ = {};
}

auto ecsact::interpret::details::on_destroy( //
	castable_destroyable_ids_t id,
	std::function<void()>      callback
) -> event_ref {
	auto& info = lifecycle_info[static_cast<int>(id.index())];

	auto ref = event_ref{};
	ref.destroyable_index_ = static_cast<int>(id.index());
	ref.id_ = info.gen_next_id();
	info.callbacks[ref.id_] = callback;
	info.callback_id[ref.id_] = destroyable_id_as_int(id);
	return ref;
}

auto ecsact::interpret::details::trigger_on_destroy( //
	destroyable_id_t id
) -> void {
	for(auto index : callback_indices(id)) {
		auto& info = lifecycle_info[index];
		for(auto itr = info.callback_id.begin(); itr != info.callback_id.end();) {
			auto& event_ref = itr->first;
			auto  callback_id = itr->second;
			if(callback_id == destroyable_id_as_int(id)) {
				auto callbacks_itr = info.callbacks.find(event_ref);
				if(callbacks_itr != info.callbacks.end()) {
					callbacks_itr->second();
					info.callbacks.erase(callbacks_itr);
				}
				itr = info.callback_id.erase(itr);
			} else {
				++itr;
			}
		}
	}
}
