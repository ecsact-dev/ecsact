#pragma once

#include <cstdint>
#include "ecsact/runtime/common.h"
#include <entt/entity/entity.hpp>

namespace ecsact::entt {

/**
 * The Ecsact and EnTT entity IDs are supposed to be 1:1. This class serves to
 * make using either trivial.
 */
class entity_id {
	std::int32_t _id;

public:
	inline entity_id() {
	}

	inline entity_id(::entt::entity entt_entity)
		: _id(reinterpret_cast<std::int32_t&>(entt_entity)) {
	}

	inline entity_id(ecsact_entity_id ecsact_entity)
		: _id(reinterpret_cast<std::int32_t&>(ecsact_entity)) {
	}

	entity_id(const entity_id&) = default;
	entity_id(entity_id&&) = default;

	auto operator=(const entity_id&) -> entity_id& = default;
	auto operator=(entity_id&&) -> entity_id& = default;

	auto operator<=>(const entity_id&) const = default;

	inline auto operator=(::entt::entity entt_entity) noexcept -> entity_id& {
		_id = reinterpret_cast<std::int32_t&>(entt_entity);
		return *this;
	}

	inline auto operator=(ecsact_entity_id ecsact_entity) noexcept -> entity_id& {
		_id = reinterpret_cast<std::int32_t&>(ecsact_entity);
		return *this;
	}

	inline operator ecsact_entity_id() const noexcept {
		return reinterpret_cast<const ecsact_entity_id&>(_id);
	}

	inline operator ::entt::entity() const noexcept {
		return reinterpret_cast<const ::entt::entity&>(_id);
	}

	[[nodiscard]] inline auto as_ecsact() const noexcept -> ecsact_entity_id {
		return reinterpret_cast<const ecsact_entity_id&>(_id);
	}

	[[nodiscard]] inline auto as_entt() const noexcept -> ::entt::entity {
		return reinterpret_cast<const ::entt::entity&>(_id);
	}
};

} // namespace ecsact::entt
