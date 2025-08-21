#pragma once

#include <string>
#include <filesystem>

namespace ecsact::cli {
auto compiler_version_string( //
	std::filesystem::path compiler_path
) -> std::string;
}
