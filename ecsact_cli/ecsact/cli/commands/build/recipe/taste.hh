#pragma once

#include <filesystem>

#include "ecsact/cli/commands/build/build_recipe.hh"

namespace ecsact::cli {
auto taste_recipe( //
	const ecsact::build_recipe& recipe,
	std::filesystem::path       output_path
) -> int;
}
