#include "ecsact/si/wasmer/detail/mem_stack.hh"

#include <cassert>
#include <cstdint>
#include <atomic>
#include <optional>
#include <span>
#include <typeindex>
#include <unordered_map>
#include <map>
#include <vector>
#include <string_view>
#include <cstring>

namespace {
struct call_mem_info_t {
	std::span<std::byte> data;
	std::size_t          data_offset = 0;
#ifndef NDEBUG
	std::map<std::size_t, const char*> data_offset_types;
	std::vector<std::string_view>      method_trace;
#endif
};

thread_local auto call_mem_info = std::optional<call_mem_info_t>{};
} // namespace

#ifndef NDEBUG
#	define ASSERT_OFFSET_TYPE(offset, type)                     \
		{                                                          \
			auto& types = call_mem_info->data_offset_types;          \
			assert(types.contains(offset));                          \
			assert(std::strcmp(types.at(offset), type.name()) == 0); \
		}                                                          \
		static_assert(true, "macro requires semi-colon")
#	define ASSIGN_OFFSET_TYPE(offset, type)                      \
		assert(!call_mem_info->data_offset_types.contains(offset)); \
		call_mem_info->data_offset_types.insert({offset, type.name()})
#else
#	define ASSERT_OFFSET_TYPE(offset, type) \
		static_assert(true, "macro requires semi-colon")
#	define ASSIGN_OFFSET_TYPE(offset, type) \
		static_assert(true, "macro requires semi-colon")
#endif

auto ecsact::wasm::detail::set_call_mem_data( //
	void*  data,
	size_t max_size
) -> void {
	if(data == nullptr) {
		call_mem_info = std::nullopt;
		return;
	}

	assert(!call_mem_info.has_value());
	auto& cmi = call_mem_info.emplace();
	cmi.data = std::span{static_cast<std::byte*>(data), max_size};
	cmi.data_offset = 0;
}

auto ecsact::wasm::detail::call_mem_alloc_raw( //
	size_t                data_size,
	const std::type_info& type
) -> std::int32_t {
	assert(call_mem_info.has_value());
	assert(data_size > 0);
	auto index = static_cast<std::int32_t>(call_mem_info->data_offset);
	ASSIGN_OFFSET_TYPE(call_mem_info->data_offset, type);
	call_mem_info->data_offset += data_size;
	assert(index >= 0);
	return index;
}

auto ecsact::wasm::detail::call_mem_read_raw( //
	std::int32_t          offset,
	const std::type_info& type
) -> void* {
	assert(call_mem_info.has_value());
	assert(offset < call_mem_info->data_offset);
	ASSERT_OFFSET_TYPE(offset, type);
	return call_mem_info->data.data() + offset;
}

auto ecsact::wasm::detail::debug_trace_method( //
	[[maybe_unused]] const char* method_name
) -> void {
#ifndef NDEBUG
	assert(call_mem_info.has_value());
	call_mem_info->method_trace.push_back(std::string_view{method_name});
#endif
}
