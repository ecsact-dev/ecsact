#pragma once

#include <variant>
#include <cassert>
#include "ecsact/runtime/common.h"

namespace ecsact::rt_entt_codegen {
struct system_like_id_variant
	: std::variant<ecsact_system_id, ecsact_action_id> {
	using variant::variant;

	operator ecsact_system_like_id() const {
		return get_sys_like_id();
	}

	inline auto as_system() const -> ecsact_system_id {
		assert(is_system());
		return std::get<ecsact_system_id>(*this);
	}

	inline auto as_action() const -> ecsact_action_id {
		assert(is_action());
		return std::get<ecsact_action_id>(*this);
	}

	inline auto is_system() const -> bool {
		return std::holds_alternative<ecsact_system_id>(*this);
	}

	inline auto is_action() const -> bool {
		return std::holds_alternative<ecsact_action_id>(*this);
	}

	inline auto get_sys_like_id() const -> ecsact_system_like_id {
		return std::visit(
			[](auto&& arg) { return ecsact_id_cast<ecsact_system_like_id>(arg); },
			*this
		);
	}
};
} // namespace ecsact::rt_entt_codegen

template<>
ECSACT_ALWAYS_INLINE ecsact_system_like_id ecsact_id_cast<
	ecsact_system_like_id,
	ecsact::rt_entt_codegen::system_like_id_variant>(
	ecsact::rt_entt_codegen::system_like_id_variant v
) {
	return v.get_sys_like_id();
}

template<>
ECSACT_ALWAYS_INLINE ecsact_decl_id
ecsact_id_cast<ecsact_decl_id, ecsact::rt_entt_codegen::system_like_id_variant>(
	ecsact::rt_entt_codegen::system_like_id_variant v
) {
	return ecsact_id_cast<ecsact_decl_id>(v.get_sys_like_id());
}
