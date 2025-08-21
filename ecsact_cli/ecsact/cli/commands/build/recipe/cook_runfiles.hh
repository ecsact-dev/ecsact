#pragma once

#include <filesystem>

namespace ecsact::cli::cook {
auto load_runfiles(const char* argv0, std::filesystem::path inc_dir) -> bool;
auto load_tracy_runfiles(const char* argv0, std::filesystem::path inc_dir)
	-> bool;
} // namespace ecsact::cli::cook
