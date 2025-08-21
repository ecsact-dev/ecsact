#pragma once

#include <type_traits>
#include <cstdint>

namespace ecsact::interpret::details {

/**
 * @NOTE: Don't use this. Use gen_next_id() instead.
 */
auto gen_next_id_() -> int32_t;

/**
 * Simple ID generation tool
 */
template<typename ID>
// light restriction on ID type. All ecsact IDs are opaque enums.
	requires(std::is_enum_v<ID>)
auto gen_next_id() -> ID {
	return static_cast<ID>(gen_next_id_());
}

} // namespace ecsact::interpret::details
