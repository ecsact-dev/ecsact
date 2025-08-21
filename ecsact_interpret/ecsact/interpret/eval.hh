#pragma once

#include <filesystem>
#include <vector>

#include "parse_eval_error.hh"

namespace ecsact {

std::vector<parse_eval_error> eval_files(
	std::vector<std::filesystem::path> files
);

} // namespace ecsact
