#pragma once

#include <cstdint>
#include <cstddef>

namespace ecsact::entt::detail {
/**
 * Opaque hash algorithm used by the Ecsact EnTT Runtime
 *
 * @param data bytes to hash
 * @param data_length length of @p data
 * @returns hash value
 */
auto bytes_hash( //
	std::byte* data,
	int        data_length
) -> std::uint64_t;
} // namespace ecsact::entt::detail
