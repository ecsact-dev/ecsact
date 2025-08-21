#pragma once

#include <vector>
#include <filesystem>
#include <optional>
#include "ecsact/codegen/plugin.h"

namespace ecsact::cli {

struct codegen_options {
	std::vector<std::filesystem::path>       plugin_paths;
	std::optional<std::filesystem::path>     outdir;
	std::optional<ecsact_codegen_write_fn_t> write_fn;
	bool                                     only_print_output_files;
};

auto codegen(codegen_options options) -> int;

} // namespace ecsact::cli
