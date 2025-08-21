#include <iostream>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include "ecsact/cli/bazel_stamp_header.hh"
#include "ecsact/cli/commands/build.hh"
#include "ecsact/cli/commands/codegen.hh"
#include "ecsact/cli/commands/recipe-bundle.hh"
#include "ecsact/cli/commands/command.hh"
#include "ecsact/cli/commands/config.hh"

using namespace std::string_view_literals;
namespace fs = std::filesystem;

constexpr auto LOGO_RAW = R"(
         /##########\
     ####  #/     \##\
    ######         \##\
     ####  ##\      \#        |@@@@
           \##\        ####   @@@@@@@@@
      @@@@@@@@@@@@@@@ ###### @@@@@@@@@@@@@@
                       ####   @@@@@@@@@
                   /#         |@@@@
         ####     /####/
        ###### ####/
         ####
)"sv;

constexpr auto USAGE = R"(Ecsact SDK Command Line

Usage:
	ecsact (--help | -h)
	ecsact (--version | -v)
	ecsact benchmark ([<options>...] | --help)
	ecsact build ([<options>...] | --help)
	ecsact codegen ([<options>...] | --help)
	ecsact config ([<options>...] | --help)
	ecsact recipe-bundle ([<options>...] | --help)
)";

std::string colorize_logo() {
	std::string logo;
	logo.reserve(LOGO_RAW.size() * 2);

	std::string curr_color;
	for(auto c : LOGO_RAW) {
		std::string color;
		if(c == '\\' || c == '|' || c == '_' || c == '/') {
			color = "\x1B[90m";
		} else if(c == '#') {
			color = "\033[36;1m";
		} else if(c == '@') {
			color = "\033[0m\033[38;5;214m";
		}

		if(!color.empty() && color != curr_color) {
			logo += color;
			curr_color = color;
		}

		logo += c;
	}

	logo += "\x1B[0m";

	return logo;
}

void print_usage() {
	std::cerr << colorize_logo() << "\n" << USAGE;
}

int main(int argc, const char* argv[]) {
	using ecsact::cli::detail::command_fn_t;

	const std::unordered_map<std::string, command_fn_t> commands{
		// {"benchmark", &ecsact::cli::detail::benchmark_command},
		{"build", &ecsact::cli::detail::build_command},
		{"codegen", &ecsact::cli::detail::codegen_command},
		{"config", &ecsact::cli::detail::config_command},
		{"recipe-bundle", &ecsact::cli::detail::recipe_bundle_command},
	};

	if(argc >= 2) {
		std::string command = argv[1];

		if(command == "-h" || command == "--help") {
			print_usage();
			return 0;
		}

		if(command == "-v" || command == "--version") {
#ifdef ECSACT_CLI_USE_SDK_VERSION
			std::cout << STABLE_ECSACT_SDK_VERSION << "\n";
			return 0;
#else
			std::cerr << "No version available in this build\n";
			return 1;
#endif
		}

		if(command.starts_with('-')) {
			std::cerr << "Expected subcommand and instead got '" << command << "'\n";
			print_usage();
			return 1;
		}

		if(!commands.contains(command)) {
			std::cerr << "Invalid subcommand: " << command << "\n";
			print_usage();
			return 2;
		}

		return commands.at(command)(argc, argv);
	}

	std::cerr << "Expected subcommand.\n";
	print_usage();
	return 1;
}
