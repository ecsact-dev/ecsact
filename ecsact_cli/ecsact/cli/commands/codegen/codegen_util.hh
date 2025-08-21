
#pragma once

#include <vector>
#include <string>
#include <filesystem>
#include <optional>

namespace ecsact::cli {

struct resolve_plugin_path_options {
	const std::string&                        plugin_arg;
	const std::filesystem::path&              default_plugins_dir;
	const std::vector<std::filesystem::path>& additional_plugin_dirs;
};

auto resolve_plugin_path( //
	const resolve_plugin_path_options&  options,
	std::vector<std::filesystem::path>& checked_plugin_paths
) -> std::optional<std::filesystem::path>;

auto resolve_plugin_path( //
	const resolve_plugin_path_options& options
) -> std::optional<std::filesystem::path>;

auto current_platform_codegen_plugin_extension() -> std::string;

auto get_default_plugins_dir() -> std::filesystem::path;

auto is_default_plugin(std::string_view plugin_arg) -> bool;

} // namespace ecsact::cli
