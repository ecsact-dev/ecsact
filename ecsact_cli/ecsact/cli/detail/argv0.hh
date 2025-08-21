#pragma once

#include <filesystem>

namespace ecsact::cli::detail {
auto canon_argv0(const char* argv0) -> std::filesystem::path;
}
