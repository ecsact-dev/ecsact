#pragma once

#include <filesystem>

namespace ecsact::cli::detail {

/**
 * The Ecsact CLI isn't built with long path awareness on Windows. This utility
 * function returns an absolute //?/ prefixed path on Windows and on other
 * platforms does nothing.
 */
auto long_path_workaround(std::filesystem::path p) -> std::filesystem::path;
} // namespace ecsact::cli::detail
