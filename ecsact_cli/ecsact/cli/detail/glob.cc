#include "ecsact/cli/detail/glob.hh"

#include <filesystem>
#include <string_view>
#include <cassert>

namespace fs = std::filesystem;
using namespace std::string_view_literals;

constexpr auto wildcard = "*"sv;
constexpr auto recursive_wildcard = "**"sv;

static auto path_ends_with(const fs::path& p, const fs::path& suffix) -> bool {
	assert(!p.empty());
	if(p.empty()) {
		return false;
	}
	if(suffix.empty()) {
		return true;
	}

	auto p_itr = --p.end();
	auto suffix_itr = --suffix.end();

	for(; p_itr != p.begin() && suffix_itr != suffix.begin();
			--p_itr, --suffix_itr) {
		if(*p_itr != *suffix_itr) {
			return false;
		}
	}

	return true;
}

static auto path_from_itrs( //
	fs::path::iterator begin,
	fs::path::iterator end
) -> fs::path {
	auto result = fs::path{};
	for(auto itr = begin; itr != end; ++itr) {
		result /= *itr;
	}
	return result;
}

static auto expand_path_globs_internal(
	fs::path               path,
	std::vector<fs::path>& out_paths,
	std::error_code&       ec
) -> void {
	ec = {};
	auto has_wildcards = false;

	for(auto itr = path.begin(); itr != path.end(); ++itr) {
		auto str = itr->string();
		auto path_prefix = path_from_itrs(path.begin(), itr);
		if(str.starts_with(recursive_wildcard)) {
			has_wildcards = true;
			auto path_suffix = path_from_itrs(std::next(itr), path.end());
			auto filename_suffix = str.substr(recursive_wildcard.size());
			for(auto entry : fs::recursive_directory_iterator(path_prefix, ec)) {
				if(path_ends_with(entry.path(), path_suffix)) {
					if(entry.path().filename().string().ends_with(filename_suffix)) {
						out_paths.push_back(entry.path());
					}
				}
			}

			if(ec) {
				return;
			}
		} else if(str.starts_with(wildcard)) {
			has_wildcards = true;
			auto path_suffix = path_from_itrs(std::next(itr), path.end());
			auto filename_suffix = str.substr(wildcard.size());
			for(auto entry : fs::directory_iterator(path_prefix, ec)) {
				if(path_ends_with(entry.path(), path_suffix)) {
					if(entry.path().filename().string().ends_with(filename_suffix)) {
						out_paths.push_back(entry.path());
					}
				}
			}

			if(ec) {
				return;
			}
		}
	}

	if(ec) {
		return;
	}

	if(!has_wildcards) {
		out_paths.push_back(path);
	}
}

auto ecsact::cli::detail::expand_path_globs( //
	fs::path         p,
	std::error_code& ec
) -> std::vector<fs::path> {
	auto result = std::vector<fs::path>{};
	expand_path_globs_internal(p, result, ec);
	return result;
}

auto ecsact::cli::detail::path_matches_glob(
	const std::filesystem::path& p,
	const std::filesystem::path& glob_pattern
) -> bool {
	auto p_itr = p.begin();
	auto glob_itr = glob_pattern.begin();

	for(; p_itr != p.end() && glob_itr != glob_pattern.end();
			++p_itr, ++glob_itr) {
		if(*p_itr == *glob_itr) {
			continue;
		}

		auto glob_comp_str = glob_itr->string();
		if(glob_comp_str.starts_with(recursive_wildcard)) {
			auto filename_suffix = glob_comp_str.substr(recursive_wildcard.size());
			if(p_itr->string().ends_with(filename_suffix)) {
				return true;
			}
		} else if(glob_comp_str.starts_with(wildcard)) {
			auto filename_suffix = glob_comp_str.substr(wildcard.size());
			if(!p_itr->string().ends_with(filename_suffix)) {
				return false;
			}
		} else {
			return false;
		}
	}

	return glob_itr == glob_pattern.end();
}

auto ecsact::cli::detail::path_before_glob(fs::path p) -> fs::path {
	auto itr = p.begin();
	for(; itr != p.end(); ++itr) {
		auto str = itr->string();
		if(str.starts_with(recursive_wildcard)) {
			break;
		}
		if(str.starts_with(wildcard)) {
			break;
		}
	}

	return path_from_itrs(p.begin(), itr);
}

auto ecsact::cli::detail::path_strip_prefix(
	const fs::path  p,
	const fs::path& strip_prefix
) -> std::optional<fs::path> {
	auto p_itr = p.begin();
	auto prefix_itr = strip_prefix.begin();

	for(; p_itr != p.end() && prefix_itr != strip_prefix.end();
			++p_itr, ++prefix_itr) {
		if(*p_itr != *prefix_itr) {
			break;
		}
	}

	if(prefix_itr == strip_prefix.end()) {
		return path_from_itrs(p_itr, p.end());
	}

	return {};
}
