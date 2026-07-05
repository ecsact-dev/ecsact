#pragma once

#include <string>
#include <optional>
#include "ecsact/runtime/common.h"

namespace ecsact::detail {
constexpr auto is_builtin_package(std::string package_name) -> bool {
	return package_name.starts_with("ecsact.") || package_name == "ecsact";
}

constexpr auto is_known_builtin_package(std::string package_name)
	-> std::optional<ecsact_package_id> {
	if(package_name == "ecsact.notify") {
		return ECSACT_BUILTIN_PKG_NOTIFY_ID;
	}

	return std::nullopt;
}
} // namespace ecsact::detail
