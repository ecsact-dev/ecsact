#pragma once

#include <type_traits>
#include "ecsact/runtime/dynamic.h"
#include "ecsact/runtime/common.h"

struct ecsact_system_execution_context;

namespace ecsact {

struct execution_context {
	[[no_unique_address]] ecsact_system_execution_context* const _ctx;

	/**
	 * Safe way to create a readonly execution context
	 */
	static ECSACT_ALWAYS_INLINE auto make_readonly( //
		const ecsact_system_execution_context* const ctx
	) -> const execution_context {
		return execution_context{
			const_cast<ecsact_system_execution_context* const>(ctx)
		};
	}

	template<typename A>
	ECSACT_ALWAYS_INLINE auto action() const -> A {
		A action;
		ecsact_system_execution_context_action(_ctx, &action);
		return action;
	}

	template<typename C, typename... AssocFields>
		requires(!std::is_empty_v<C>)
	ECSACT_ALWAYS_INLINE auto get(AssocFields&&... assoc_fields) const -> C {
		if constexpr(C::has_assoc_fields) {
			static_assert(
				sizeof...(AssocFields) > 0,
				"must be called with assoc fields"
			);
		}

		auto comp = C{};

		if constexpr(sizeof...(AssocFields) > 0) {
			const void* assoc_field_values[sizeof...(AssocFields)] = {
				&assoc_fields...,
			};

			ecsact_system_execution_context_get(
				_ctx,
				ecsact_id_cast<ecsact_component_like_id>(C::id),
				&comp,
				assoc_field_values
			);
		} else {
			ecsact_system_execution_context_get(
				_ctx,
				ecsact_id_cast<ecsact_component_like_id>(C::id),
				&comp,
				nullptr
			);
		}
		return comp;
	}

	template<typename C, typename... AssocFields>
		requires(!std::is_empty_v<C>)
	ECSACT_ALWAYS_INLINE auto update(
		const C& updated_component,
		AssocFields&&... assoc_fields
	) -> void {
		if constexpr(C::has_assoc_fields) {
			static_assert(
				sizeof...(AssocFields) > 0,
				"must be called with assoc fields"
			);
		}

		if constexpr(sizeof...(AssocFields) > 0) {
			const void* assoc_field_values[sizeof...(AssocFields)] = {
				&assoc_fields...,
			};
			ecsact_system_execution_context_update(
				_ctx,
				ecsact_id_cast<ecsact_component_like_id>(C::id),
				&updated_component,
				assoc_field_values
			);
		} else {
			ecsact_system_execution_context_update(
				_ctx,
				ecsact_id_cast<ecsact_component_like_id>(C::id),
				&updated_component,
				nullptr
			);
		}
	}

	template<typename C, typename... AssocFields>
	ECSACT_ALWAYS_INLINE auto has(AssocFields&&... assoc_fields) -> bool const {
		if constexpr(C::has_assoc_fields) {
			static_assert(
				sizeof...(AssocFields) > 0,
				"must be called with assoc fields"
			);
		}

		if constexpr(sizeof...(AssocFields) > 0) {
			const void* assoc_field_values[sizeof...(AssocFields)] = {
				&assoc_fields...,
			};
			return ecsact_system_execution_context_has(
				_ctx,
				ecsact_id_cast<ecsact_component_like_id>(C::id),
				assoc_field_values
			);
		} else {
			return ecsact_system_execution_context_has(
				_ctx,
				ecsact_id_cast<ecsact_component_like_id>(C::id),
				nullptr
			);
		}
	}

	template<typename C, typename... AssocFields>
	ECSACT_ALWAYS_INLINE auto stream_toggle(
		bool enable_stream_data,
		AssocFields&&... assoc_fields
	) -> void {
		if constexpr(C::has_assoc_fields) {
			static_assert(
				sizeof...(AssocFields) > 0,
				"must be called with assoc fields"
			);
		}
		if constexpr(sizeof...(AssocFields) > 0) {
			const void* assoc_field_values[sizeof...(AssocFields)] = {
				&assoc_fields...,
			};
			ecsact_system_execution_context_stream_toggle(
				_ctx,
				C::id,
				enable_stream_data,
				assoc_field_values
			);
		} else {
			ecsact_system_execution_context_stream_toggle(
				_ctx,
				C::id,
				enable_stream_data,
				nullptr
			);
		}
	}

	template<typename C>
		requires(!std::is_empty_v<C>)
	ECSACT_ALWAYS_INLINE auto add(const C& new_component) -> void {
		ecsact_system_execution_context_add(
			_ctx,
			ecsact_id_cast<ecsact_component_like_id>(C::id),
			&new_component
		);
	}

	template<typename C>
		requires(std::is_empty_v<C>)
	ECSACT_ALWAYS_INLINE auto add() -> void {
		ecsact_system_execution_context_add(
			_ctx,
			ecsact_id_cast<ecsact_component_like_id>(C::id),
			nullptr
		);
	}

	template<typename C>
	ECSACT_ALWAYS_INLINE auto remove() -> void {
		ecsact_system_execution_context_remove(
			_ctx,
			ecsact_id_cast<ecsact_component_like_id>(C::id),
			nullptr
		);
	}

	template<typename... C>
	ECSACT_ALWAYS_INLINE auto generate(C&&... components) -> void {
		ecsact_component_id component_ids[]{C::id...};
		const void*         components_data[]{&components...};

		ecsact_system_execution_context_generate(
			_ctx,
			sizeof...(C),
			component_ids,
			components_data
		);
	}

	ECSACT_ALWAYS_INLINE auto parent() const -> const execution_context {
		return execution_context::make_readonly(
			ecsact_system_execution_context_parent(_ctx)
		);
	}

	ECSACT_ALWAYS_INLINE auto same(const execution_context& other) const -> bool {
		return ecsact_system_execution_context_same(_ctx, other._ctx);
	}

	ECSACT_ALWAYS_INLINE auto other( //
		ecsact_system_assoc_id assoc_id
	) -> execution_context {
		return execution_context{
			ecsact_system_execution_context_other(_ctx, assoc_id)
		};
	}

	ECSACT_ALWAYS_INLINE auto id() const -> ecsact_system_like_id {
		return ecsact_system_execution_context_id(_ctx);
	}

	ECSACT_ALWAYS_INLINE auto entity() const -> ecsact_entity_id {
		return ecsact_system_execution_context_entity(_ctx);
	}
};

} // namespace ecsact
