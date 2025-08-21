#include "ecsact/cli/detail/argv0.hh"

#include <filesystem>
#include "ecsact/cli/detail/executable_path/executable_path.hh"

namespace fs = std::filesystem;

auto ecsact::cli::detail::canon_argv0( //
	const char* argv0
) -> std::filesystem::path {
	auto exec_path = executable_path::executable_path();
	if(exec_path.empty()) {
		exec_path = fs::path{argv0};
		auto ec = std::error_code{};
		exec_path = fs::canonical(exec_path, ec);
		if(ec) {
			exec_path = fs::weakly_canonical(fs::path{argv0});
		}
	}

	return exec_path;
}
