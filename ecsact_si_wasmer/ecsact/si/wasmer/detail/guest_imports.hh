#pragma once

#include <string_view>
#include <functional>
#include <map>
#include <wasm.h>
#include "ecsact/si/wasmer/detail/minst/minst.hh"

namespace ecsact::wasm::detail {
using allowed_guest_imports_t = std::unordered_map<
	std::string_view, // Function name
	std::function<minst_import_resolve_func()>>;

using allowed_guest_modules_t = std::unordered_map<
	std::string_view, // Module name
	allowed_guest_imports_t>;
} // namespace ecsact::wasm::detail
