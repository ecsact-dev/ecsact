#pragma once

#include <optional>
#include <filesystem>
#include <string>
#include <cstdint>
#include <format>
#include "magic_enum.hpp"
#include "ecsact/cli/commands/build/cc_compiler_config.hh"

namespace ecsact::cli {
/// Find a compiler with the given name or path
auto find_cc_compiler(
	std::filesystem::path work_dir,
	std::string           compiler_name_or_path
) -> std::optional<cc_compiler>;

/// Automatically detect a C++ compiler without input
auto detect_cc_compiler( //
	std::filesystem::path work_dir
) -> std::optional<cc_compiler>;
} // namespace ecsact::cli
