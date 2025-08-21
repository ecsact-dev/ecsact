#pragma once

#include <string>
#include <vector>

namespace ecsact::cli {
auto cc_defines_gen( //
	std::vector<std::string> imports,
	std::vector<std::string> exports
) -> std::vector<std::string>;
} // namespace ecsact::cli
