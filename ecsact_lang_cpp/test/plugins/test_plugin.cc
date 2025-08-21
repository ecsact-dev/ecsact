#include <string>
#include <format>
#include <iostream>
#include <filesystem>
#include <cstdlib>

namespace fs = std::filesystem;

auto main(int argc, char* argv[]) -> int {
	auto ecsact_cli = std::getenv("ECSACT_CLI");
	auto ecsact_srcs = std::getenv("ECSACT_SRCS");
	auto ecsact_codegen_plugin = std::getenv("ECSACT_CODEGEN_PLUGIN");
	auto outdir = std::getenv("BUILD_WORKING_DIRECTORY")
		? fs::path(std::getenv("BUILD_WORKING_DIRECTORY")) / "test" / "plugins" /
			"_test_out"
		: fs::absolute(fs::path{"_test_out"});

	std::cout << std::format( //
		"ECSACT_CLI: {}\n"
		"ECSACT_SRCS: {}\n"
		"ECSACT_CODEGEN_PLUGIN: {}\n"
		"OUTDIR: {}\n"
		"CURRENT_DIRECTORY: {}\n",
		ecsact_cli,
		ecsact_srcs,
		ecsact_codegen_plugin,
		outdir.string(),
		fs::current_path().string()
	);

	auto cmd_str = std::format( //
		"{} codegen {} --plugin={} --outdir={}",
		fs::absolute(ecsact_cli).string(),
		ecsact_srcs,
		ecsact_codegen_plugin,
		outdir.string()
	);

	std::cout << cmd_str << "\n";

	auto exit_code = std::system(cmd_str.c_str());

	std::cout << "Exited with code " << exit_code << "\n";

	return exit_code;
}
