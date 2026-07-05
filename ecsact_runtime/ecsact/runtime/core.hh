#pragma once

#include <type_traits>
#include <vector>
#include <map>
#include <functional>
#include <optional>
#include <cassert>
#include "ecsact/runtime/core.h"

namespace ecsact::core {

class builder_entity {
	friend class execution_options;

public:
	template<typename C>
	ECSACT_ALWAYS_INLINE auto add_component(C* component) -> builder_entity& {
		components.push_back(
			ecsact_component{
				.component_id = C::id,
				.component_data = component,
			}
		);
		return *this;
	}

private:
	ecsact_placeholder_entity_id  placeholder_entity_id;
	std::vector<ecsact_component> components;
};

class execution_options {
public:
	execution_options() = default;
	execution_options(const execution_options&) = default;
	execution_options(execution_options&&) = default;

	/**
	 * The lifetime of @p `component` must be maintained until the
	 * `ecsact::core::execution_options` destructor occurs or `clear()` occurs.
	 */
	template<typename C>
	void add_component(ecsact_entity_id entity, C* component) {
		add_components_container.push_back(
			ecsact_component{.component_id = C::id, .component_data = component}
		);
		add_entities_container.push_back(entity);
	}

	/**
	 * The lifetime of @p `component` must be maintained until the
	 * `ecsact::core::execution_options` destructor occurs or `clear()` occurs.
	 */
	template<typename C>
	void update_component(ecsact_entity_id entity, C* component) {
		update_components_container.push_back(
			ecsact_component{.component_id = C::id, .component_data = component}
		);
		update_entities_container.push_back(entity);
	}

	template<typename C>
	ECSACT_ALWAYS_INLINE void remove_component(ecsact_entity_id entity_id) {
		remove_component_ids_container.push_back(C::id);
		remove_entities_container.push_back(entity_id);
	}

	ECSACT_ALWAYS_INLINE void remove_component(
		ecsact_entity_id    entity_id,
		ecsact_component_id component_id
	) {
		remove_component_ids_container.push_back(component_id);
		remove_entities_container.push_back(entity_id);
	}

	ECSACT_ALWAYS_INLINE auto create_entity(
		ecsact_placeholder_entity_id placeholder_entity_id = {}
	) -> builder_entity& {
		auto& builder = create_entities.emplace_back();
		builder.placeholder_entity_id = placeholder_entity_id;
		return builder;
	}

	ECSACT_ALWAYS_INLINE void destroy_entity(const ecsact_entity_id& entity_id) {
		destroy_entities.push_back(entity_id);
	}

	/**
	 * The lifetime of @p `action` must be maintained until the
	 * `ecsact::core::execution_options` destructor occurs or `clear()` occurs.
	 */
	template<typename Action>
	ECSACT_ALWAYS_INLINE void push_action(const Action* action) {
		actions.push_back(ecsact_action{Action::id, action});
	}

	ECSACT_ALWAYS_INLINE void push_action_raw(ecsact_action action) {
		actions.push_back(action);
	}

	ECSACT_ALWAYS_INLINE void clear() {
		add_entities_container.clear();
		add_components_container.clear();

		update_entities_container.clear();
		update_components_container.clear();

		remove_entities_container.clear();
		remove_component_ids_container.clear();

		entities_components.clear();
		entities_component_lengths.clear();

		create_entities.clear();
		destroy_entities.clear();

		actions.clear();
	}

	ECSACT_ALWAYS_INLINE auto c() -> ecsact_execution_options {
		auto options = ecsact_execution_options{};

		options.add_components_length = add_components_container.size();
		options.add_components_entities = add_entities_container.data();
		options.add_components = add_components_container.data();

		options.update_components_length = update_components_container.size();
		options.update_components_entities = update_entities_container.data();
		options.update_components = update_components_container.data();

		options.remove_components_length = remove_component_ids_container.size();
		options.remove_components_entities = remove_entities_container.data();
		options.remove_components = remove_component_ids_container.data();

		entities_components.clear();
		entities_components.reserve(create_entities.size());
		entities_component_lengths.clear();
		entities_component_lengths.reserve(create_entities.size());
		create_placeholders.clear();
		create_placeholders.reserve(create_entities.size());
		for(auto& built_entity : create_entities) {
			create_placeholders.push_back(built_entity.placeholder_entity_id);
			entities_component_lengths.push_back(built_entity.components.size());
			entities_components.push_back(built_entity.components.data());
		}

		options.create_entities = create_placeholders.data();
		options.create_entities_components = entities_components.data();
		options.create_entities_length = create_entities.size();
		options.create_entities_components_length =
			entities_component_lengths.data();

		options.destroy_entities = destroy_entities.data();
		options.destroy_entities_length = destroy_entities.size();

		options.actions = actions.data();
		options.actions_length = actions.size();

		return options;
	}

private:
	std::vector<ecsact_entity_id> add_entities_container;
	std::vector<ecsact_component> add_components_container;

	std::vector<ecsact_entity_id> update_entities_container;
	std::vector<ecsact_component> update_components_container;

	std::vector<ecsact_entity_id>    remove_entities_container;
	std::vector<ecsact_component_id> remove_component_ids_container;

	std::vector<builder_entity>   create_entities;
	std::vector<ecsact_entity_id> destroy_entities;

	std::vector<ecsact_placeholder_entity_id> create_placeholders;
	std::vector<int32_t>                      entities_component_lengths;
	std::vector<ecsact_component*>            entities_components;

	std::vector<ecsact_action> actions;
};

class registry {
	ecsact_registry_id _id;
	bool               _owned = false;

public:
	registry(const char* name) {
		_id = ecsact_create_registry(name);
		_owned = true;
	}

	explicit registry(ecsact_registry_id existing_registry_id) {
		_id = existing_registry_id;
		_owned = false;
	}

	registry(registry&& other) : _id(other._id), _owned(other._owned) {
		other._owned = false;
	}

	~registry() {
		if(_owned) {
			ecsact_destroy_registry(_id);
		}

		_owned = false;
	}

	registry& operator=(registry&& other) {
		_id = other._id;
		_owned = other._owned;
		other._owned = false;
		return *this;
	}

	bool operator==(const registry& other) {
		return _id == other._id;
	}

	ecsact_registry_id id() const noexcept {
		return _id;
	}

	auto clear() -> void {
		ecsact_clear_registry(_id);
	}

	auto empty() const -> bool {
		return ecsact_count_entities(_id) == 0;
	}

	auto create_entity() {
		return ecsact_create_entity(_id);
	}

	ECSACT_ALWAYS_INLINE auto clone(const char* name) const -> registry {
		auto cloned_registry_id = ecsact_clone_registry(_id, name);
		auto cloned_registry = registry{cloned_registry_id};
		cloned_registry._owned = true;
		return cloned_registry;
	}

	ECSACT_ALWAYS_INLINE auto copy_to(const registry& dest) const -> void {
		ecsact_copy_registry(_id, dest.id());
	}

	ECSACT_ALWAYS_INLINE auto hash() const -> uint64_t {
		return ecsact_hash_registry(_id);
	}

	template<typename Component, typename... AssocFields>
		requires(!std::is_empty_v<Component>)
	ECSACT_ALWAYS_INLINE auto get_component( //
		ecsact_entity_id entity_id,
		AssocFields&&... assoc_fields
	) -> const Component& {
		if constexpr(Component::has_assoc_fields) {
			static_assert(
				sizeof...(AssocFields) > 0,
				"must be called with assoc fields"
			);
		}

		if constexpr(sizeof...(AssocFields) > 0) {
			const void* assoc_field_values[sizeof...(AssocFields)] = {
				&assoc_fields...,
			};

			return *reinterpret_cast<const Component*>(
				ecsact_get_component(_id, entity_id, Component::id, assoc_field_values)
			);
		} else {
			return *reinterpret_cast<const Component*>(
				ecsact_get_component(_id, entity_id, Component::id, nullptr)
			);
		}
	}

	template<typename Component, typename... AssocFields>
	ECSACT_ALWAYS_INLINE bool has_component(
		ecsact_entity_id entity_id,
		AssocFields&&... assoc_fields
	) {
		if constexpr(Component::has_assoc_fields) {
			static_assert(
				sizeof...(AssocFields) > 0,
				"must be called with assoc fields"
			);
		}

		if constexpr(sizeof...(AssocFields) > 0) {
			const void* assoc_field_values[sizeof...(AssocFields)] = {
				&assoc_fields...,
			};
			return ecsact_has_component(
				_id,
				entity_id,
				Component::id,
				assoc_field_values
			);
		} else {
			return ecsact_has_component(_id, entity_id, Component::id, nullptr);
		}
	}

	template<typename Component>
		requires(std::is_empty_v<Component>)
	ECSACT_ALWAYS_INLINE auto add_component(ecsact_entity_id entity_id) {
		return ecsact_add_component(_id, entity_id, Component::id, nullptr);
	}

	template<typename Component>
	ECSACT_ALWAYS_INLINE auto add_component(
		ecsact_entity_id entity_id,
		const Component& component
	) {
		if constexpr(std::is_empty_v<Component>) {
			return ecsact_add_component(_id, entity_id, Component::id, nullptr);
		} else {
			return ecsact_add_component(_id, entity_id, Component::id, &component);
		}
	}

	template<typename Component, typename... AssocFields>
	ECSACT_ALWAYS_INLINE auto update_component(
		ecsact_entity_id entity_id,
		const Component& component,
		AssocFields&&... assoc_fields
	) {
		if constexpr(Component::has_assoc_fields) {
			static_assert(
				sizeof...(AssocFields) > 0,
				"must be called with assoc fields"
			);
		}

		if constexpr(sizeof...(AssocFields) > 0) {
			const void* assoc_field_values[sizeof...(AssocFields)] = {
				&assoc_fields...,
			};
			return ecsact_update_component(
				_id,
				entity_id,
				Component::id,
				&component,
				assoc_field_values
			);
		} else {
			return ecsact_update_component(
				_id,
				entity_id,
				Component::id,
				&component,
				nullptr
			);
		}
	}

	template<typename Component, typename... AssocFields>
	ECSACT_ALWAYS_INLINE auto remove_component(
		ecsact_entity_id entity_id,
		AssocFields&&... assoc_fields
	) -> void {
		if constexpr(Component::has_assoc_fields) {
			static_assert(
				sizeof...(AssocFields) > 0,
				"must be called with assoc fields"
			);
		}

		if constexpr(sizeof...(AssocFields) > 0) {
			const void* assoc_field_values[sizeof...(AssocFields)] = {
				&assoc_fields...,
			};

			return ecsact_remove_component(
				_id,
				entity_id,
				Component::id,
				assoc_field_values
			);
		} else {
			return ecsact_remove_component(_id, entity_id, Component::id, nullptr);
		}
	}

	ECSACT_ALWAYS_INLINE auto count_entities() const -> int32_t {
		return ecsact_count_entities(_id);
	}

	ECSACT_ALWAYS_INLINE auto get_entities() const
		-> std::vector<ecsact_entity_id> {
		const auto entities_count = count_entities();
		auto       entities = std::vector<ecsact_entity_id>{};
		entities.resize(entities_count);
		ecsact_get_entities(_id, entities_count, entities.data(), nullptr);
		return entities;
	}

	ECSACT_ALWAYS_INLINE auto count_components( //
		ecsact_entity_id entity
	) const -> int32_t {
		return ecsact_count_components(_id, entity);
	}

	/**
	 * Execute systems @p execution_count times.
	 */
	auto execute_systems(int32_t execution_count = 1) -> void {
		ecsact_execute_systems(_id, execution_count, nullptr);
	}

	/**
	 * Execute systems with options. Executes systems once for each element
	 * in @p execution_options range.
	 */
	template<typename ExecutionOptionsRange>
	[[nodiscard]] auto execute_systems( //
		ExecutionOptionsRange&& execution_options
	) -> ecsact_execute_systems_error {
		auto        execution_count = std::size(execution_options);
		const auto* execution_options_list_data = std::data(execution_options);

		const ecsact_execution_options* c_execution_options_list = nullptr;
		if constexpr(
			std::is_same_v<
				decltype(execution_options_list_data),
				decltype(c_execution_options_list)>
		) {
			return ecsact_execute_systems(
				_id,
				static_cast<int32_t>(execution_count),
				execution_options_list_data
			);
		} else {
			auto execution_options_vec = std::vector<ecsact_execution_options>{};
			execution_options_vec.reserve(execution_count);
			for(auto& el : execution_options) {
				execution_options_vec.push_back(el.c());
			}
			return ecsact_execute_systems(
				_id,
				static_cast<int32_t>(execution_count),
				execution_options_vec.data()
			);
		}
	}
};

} // namespace ecsact::core
