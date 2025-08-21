
#pragma once

#include <vector>
#include <filesystem>
#include <optional>

namespace ecsact::cli::detail {
/**
 * Expands '*' and '**' inside a path to get a list of paths. '*' meaning files
 * in the one directory and '**' meaning files in multiple (recursive)
 * directories.
 */
auto expand_path_globs( //
	std::filesystem::path p,
	std::error_code&      ec
) -> std::vector<std::filesystem::path>;

auto path_matches_glob(
	const std::filesystem::path& p,
	const std::filesystem::path& glob_pattern
) -> bool;

#ifdef __clang__
#	pragma clang diagnostic push
#	pragma clang diagnostic ignored "-Wcomment"
#endif
/**
 * Gets the path before the first glob pattern. For example if your glob pattern
 * is `a/b/*` the returning path would be `a/b`. Or if you have multiple glob
 * patterns such as `a/b/*.txt/c/*.d` the returning path would still be `a/b`.
 */
auto path_before_glob(std::filesystem::path) -> std::filesystem::path;
#ifdef __clang__
#	pragma clang diagnostic pop
#endif

// not really a glob feature, but useful path util
auto path_strip_prefix(
	const std::filesystem::path  p,
	const std::filesystem::path& strip_prefix
) -> std::optional<std::filesystem::path>;
} // namespace ecsact::cli::detail
