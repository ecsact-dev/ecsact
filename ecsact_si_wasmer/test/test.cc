#include <string>
#include <cstdlib>
#include <iostream>
#include <filesystem>

using namespace std::string_view_literals;
namespace fs = std::filesystem;

constexpr auto EXE_PARAM_PREFIX = "--exe="sv;

int main(int argc, char* argv[]) {
	fs::path    exe;
	std::string cmd;

	for(int i = 1; argc > i; ++i) {
		std::string arg(argv[i]);
		if(arg.starts_with(EXE_PARAM_PREFIX)) {
			exe = fs::path(arg.substr(EXE_PARAM_PREFIX.size()));
		} else if(arg.starts_with('-')) {
			std::cerr << "[ERROR] Invalid argument: " << arg << "\n";
			return 1;
		} else {
			cmd += " " + arg;
		}
	}

	if(exe.empty()) {
		std::cerr << "[ERROR] Missing --exe argument\n";
		return 1;
	}

	cmd = ".\\" + exe.make_preferred().string() + cmd;

	std::cout << "[INFO] SUBCOMMAND: " << cmd << "\n";
	return std::system(cmd.c_str());
}
