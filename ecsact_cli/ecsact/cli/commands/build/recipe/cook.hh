#pragma once

#include <filesystem>
#include <optional>
#include "ecsact/cli/commands/build/build_recipe.hh"
#include "ecsact/cli/commands/build/cc_compiler_config.hh"

namespace ecsact::cli {

struct cook_recipe_options {
	std::vector<std::filesystem::path> files;
	std::filesystem::path              work_dir;
	std::filesystem::path              output_path;
	bool                               debug;
	bool                               tracy;

	/** Other directories to check for codegen plugins */
	std::vector<std::filesystem::path> additional_plugin_dirs;
};

/**
 * @returns the cooked runtime path
 */
auto cook_recipe(
	const char*                 argv0,
	const ecsact::build_recipe& recipe,
	cc_compiler                 compiler,
	const cook_recipe_options&  recipe_options
) -> std::optional<std::filesystem::path>;

} // namespace ecsact::cli
