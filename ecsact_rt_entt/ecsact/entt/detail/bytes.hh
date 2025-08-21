#pragma once

#include <concepts>
#include <array>
#include <bit>
#include <type_traits>
#include <cstddef>

namespace ecsact::entt::detail {

template<typename T>
	requires(std::integral<T> || std::floating_point<T>)
constexpr auto bytes_sizeof() -> int {
	using value_type = std::remove_cvref_t<T>;

	if constexpr(std::is_same_v<value_type, bool>) {
		return 1;
	} else {
		return sizeof(value_type);
	}
}

template<typename T>
struct unsigned_bit_size_equivalent;

template<typename T>
	requires(bytes_sizeof<T>() == 1)
struct unsigned_bit_size_equivalent<T> : std::type_identity<uint8_t> {};

template<typename T>
	requires(bytes_sizeof<T>() == 2)
struct unsigned_bit_size_equivalent<T> : std::type_identity<uint16_t> {};

template<typename T>
	requires(bytes_sizeof<T>() == 4)
struct unsigned_bit_size_equivalent<T> : std::type_identity<uint32_t> {};

template<typename T>
	requires(bytes_sizeof<T>() == 8)
struct unsigned_bit_size_equivalent<T> : std::type_identity<uint64_t> {};

template<typename T>
using unsigned_bit_size_equivalent_t = unsigned_bit_size_equivalent<T>::type;

template<typename T>
	requires(std::integral<T> || std::floating_point<T>)
auto bytes_copy_into( //
	T     v,
	auto& out_bytes,
	auto& out_bytes_offset = 0
) -> void {
	using value_type = std::remove_cvref_t<T>;
	constexpr auto value_size = bytes_sizeof<T>();

	auto v_bits = std::bit_cast<unsigned_bit_size_equivalent_t<T>>(v);

	if constexpr(std::is_same_v<value_type, bool>) {
		out_bytes[out_bytes_offset++] = static_cast<std::byte>(v_bits ? 1 : 0);
	} else if constexpr(value_size == 1) {
		out_bytes[out_bytes_offset++] = static_cast<std::byte>(v_bits);
	} else if constexpr(value_size == 2) {
		out_bytes[out_bytes_offset++] =
			static_cast<std::byte>((v_bits >> 8) & 0xFF);
		out_bytes[out_bytes_offset++] = static_cast<std::byte>(v_bits & 0xFF);
	} else if constexpr(value_size == 4) {
		out_bytes[out_bytes_offset++] =
			static_cast<std::byte>((v_bits >> 24) & 0xFF);
		out_bytes[out_bytes_offset++] =
			static_cast<std::byte>((v_bits >> 16) & 0xFF);
		out_bytes[out_bytes_offset++] =
			static_cast<std::byte>((v_bits >> 8) & 0xFF);
		out_bytes[out_bytes_offset++] = static_cast<std::byte>(v_bits & 0xFF);
	} else if constexpr(value_size == 8) {
		out_bytes[out_bytes_offset++] =
			static_cast<std::byte>((v_bits >> 56) & 0xFF);
		out_bytes[out_bytes_offset++] =
			static_cast<std::byte>((v_bits >> 48) & 0xFF);
		out_bytes[out_bytes_offset++] =
			static_cast<std::byte>((v_bits >> 40) & 0xFF);
		out_bytes[out_bytes_offset++] =
			static_cast<std::byte>((v_bits >> 32) & 0xFF);
		out_bytes[out_bytes_offset++] =
			static_cast<std::byte>((v_bits >> 24) & 0xFF);
		out_bytes[out_bytes_offset++] =
			static_cast<std::byte>((v_bits >> 16) & 0xFF);
		out_bytes[out_bytes_offset++] =
			static_cast<std::byte>((v_bits >> 8) & 0xFF);
		out_bytes[out_bytes_offset++] = static_cast<std::byte>(v_bits & 0xFF);
	}

	static_assert(value_size <= 8);
}

template<typename... T>
auto bytes_copy( //
	T... values
) -> std::array<std::byte, (0 + ... + bytes_sizeof<T>())> {
	auto result = std::array<std::byte, (0 + ... + bytes_sizeof<T>())>{};
	auto offset = 0;
	(bytes_copy_into(values, result, offset), ...);
	return result;
}

} // namespace ecsact::entt::detail
