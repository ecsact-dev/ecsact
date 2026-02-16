#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <format>
#include <filesystem>
#include <fstream>
#include "ecsact/cli/commands/codegen.hh"
#include "tools/cpp/runfiles/runfiles.h"

using bazel::tools::cpp::runfiles::Runfiles;
using ecsact::cli::detail::codegen_command;

using namespace std::string_literals;
namespace fs = std::filesystem;

TEST(Codegen, Success) {
	auto runfiles_err = std::string{};
	auto runfiles = Runfiles::CreateForTest(&runfiles_err);
	auto test_ecsact_cli = std::getenv("TEST_ECSACT_CLI");
	auto test_codegen_plugin_path = std::getenv("TEST_CODEGEN_PLUGIN_PATH");
	auto test_ecsact_file_path = std::getenv("TEST_ECSACT_FILE_PATH");

	ASSERT_NE(test_ecsact_cli, nullptr);
	ASSERT_NE(test_codegen_plugin_path, nullptr);
	ASSERT_NE(test_ecsact_file_path, nullptr);
	ASSERT_NE(runfiles, nullptr) << runfiles_err;

	auto generated_file_path = fs::path{"_test_codegen_outdir/test.ecsact.txt"};

	if(fs::exists(generated_file_path)) {
		fs::remove(generated_file_path);
	}

	ASSERT_TRUE(fs::exists(test_codegen_plugin_path));
	ASSERT_TRUE(fs::exists(test_ecsact_file_path));

#if _WIN32
	// TODO: this doesn't work on linux
	auto exit_code = codegen_command(
		std::vector{
			"ecsact"s,
			"codegen"s,
			std::string{test_ecsact_file_path},
			std::format("--plugin={}", test_codegen_plugin_path),
			"--outdir=_test_codegen_outdir"s,
		}
	);
#else
	auto cmd = std::format(
		"{} codegen {} --plugin={} --outdir=_test_codegen_outdir",
		test_ecsact_cli,
		std::string{test_ecsact_file_path},
		test_codegen_plugin_path,
		"_test_codegen_outdir"
	);
	auto exit_code = std::system(cmd.c_str());
#endif

	ASSERT_EQ(exit_code, 0);

	ASSERT_TRUE(fs::exists(generated_file_path));
	auto generated_file = std::ifstream{generated_file_path};
	auto generated_file_contents = std::string{
		std::istreambuf_iterator<char>(generated_file),
		std::istreambuf_iterator<char>(),
	};

	ASSERT_EQ(generated_file_contents, "this is just a test, breathe!")
		<< "Unexpected content at "
		<< fs::absolute(generated_file_path).generic_string();
}
