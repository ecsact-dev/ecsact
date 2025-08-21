#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace ecsact::cli::detail {

struct get_ecsact_modules_result {
	std::unordered_map<std::string, std::vector<std::string>> module_methods;
	std::vector<std::string> unknown_module_methods;
};

/**
 * get all the modules these methods are a part of
 */
auto get_ecsact_modules( //
	std::vector<std::string> methods
) -> get_ecsact_modules_result;

} // namespace ecsact::cli::detail
