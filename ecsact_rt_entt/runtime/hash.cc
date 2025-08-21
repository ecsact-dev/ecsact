#include "ecsact/entt/detail/hash.hh"
#include "xxhash.h"

auto ecsact::entt::detail::bytes_hash( //
	std::byte* data,
	int        data_length
) -> std::uint64_t {
	XXH64_hash_t hash = XXH3_64bits(data, data_length);
	return hash;
}
