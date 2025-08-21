#pragma once

#include <type_traits>
#include <concepts>
#include <array>

namespace ecsact::entt::detail {

/**
 *
 */
template<typename KeyT, typename ValueT, KeyT MinKeyValue, KeyT MaxKeyValue>
class id_map {
public:
	using key_type = KeyT;
	using value_type = ValueT;
	using underlying_key_type = std::underlying_type_t<key_type>;

	static constexpr auto min_key_value() -> underlying_key_type {
		return static_cast<underlying_key_type>(MinKeyValue);
	}

	static constexpr auto max_key_value() -> underlying_key_type {
		return static_cast<underlying_key_type>(MaxKeyValue);
	}

	static_assert(
		max_key_value() > min_key_value(),
		"id_map MaxKeyValue must be > MinKeyValue"
	);

	std::array<ValueT, max_key_value() - min_key_value()> _data;

	constexpr auto key_index(const key_type& key) const -> size_t {
		auto index = static_cast<size_t>(key);
		index -= static_cast<size_t>(min_key_value());
		return index;
	}

	constexpr auto operator[](const key_type& key) -> value_type& {
		return _data[key_index(key)];
	}

	constexpr auto operator[](const key_type& key) const -> const value_type& {
		return _data[key_index(key)];
	}
};
} // namespace ecsact::entt::detail
