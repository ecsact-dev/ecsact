#include "ecsact/cli/detail/long_path_workaround.hh"

auto ecsact::cli::detail::long_path_workaround( //
	std::filesystem::path p
) -> std::filesystem::path {
#if _WIN32
	auto win_path_str = "\\\\?\\" + std::filesystem::absolute(p).string();
	for(auto& c : win_path_str) {
		if(c == '/') {
			c = '\\';
		}
	}
	return win_path_str;
#else
	return p;
#endif
}
