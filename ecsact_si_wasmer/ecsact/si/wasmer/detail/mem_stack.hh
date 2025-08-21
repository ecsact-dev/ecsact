#pragma once

#include <cstdint>
#include <string_view>
#include <type_traits>
#include <typeinfo>

namespace ecsact::wasm::detail {
auto set_call_mem_data( //
	void*  data,
	size_t max_size
) -> void;

auto debug_trace_method( //
	const char* method_name
) -> void;

auto call_mem_alloc_raw( //
	size_t                data_size,
	const std::type_info& type
) -> std::int32_t;

auto call_mem_read_raw( //
	std::int32_t          offset,
	const std::type_info& type
) -> void*;

template<typename T>
	requires(std::is_trivial_v<T>)
auto call_mem_read(std::int32_t offset) -> T& {
	return *static_cast<T*>(call_mem_read_raw(offset, typeid(T)));
}

template<typename T>
	requires(std::is_trivial_v<T>)
auto call_mem_alloc(T value) -> std::int32_t {
	auto offset = call_mem_alloc_raw(sizeof(T), typeid(T));
	call_mem_read<T>(offset) = value;
	return offset;
}

template<typename T>
	requires(std::is_trivial_v<T>)
auto call_mem_replace(std::int32_t offset, T value) -> void {
	call_mem_read<T>(offset, typeid(T)) = value;
}
} // namespace ecsact::wasm::detail
