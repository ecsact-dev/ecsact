
#pragma once

#include <type_traits>
#include <vector>
#include <string>

#include "./command.hh"

namespace ecsact::cli::detail {

int recipe_bundle_command(int argc, const char* argv[]);
static_assert(std::is_same_v<command_fn_t, decltype(&recipe_bundle_command)>);

inline auto recipe_bundle_command(std::vector<std::string> args) -> int {
	auto c_args = std::vector<const char*>();
	c_args.reserve(args.size());
	for(auto& arg : args) {
		c_args.emplace_back(arg.c_str());
	}
	return recipe_bundle_command(c_args.size(), c_args.data());
}
} // namespace ecsact::cli::detail
