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

	auto generated_txt_file_path = fs::path{"_test_codegen_outdir/test.txt"};
	auto generated_zomsky_file_path =
		fs::path{"_test_codegen_outdir/test.zomsky"};

	if(fs::exists(generated_txt_file_path)) {
		fs::remove(generated_txt_file_path);
	}

	if(fs::exists(generated_zomsky_file_path)) {
		fs::remove(generated_zomsky_file_path);
	}

	ASSERT_TRUE(fs::exists(test_codegen_plugin_path))
		<< "Cannot find test plugin: "
		<< fs::absolute(test_codegen_plugin_path).string();
	ASSERT_TRUE(fs::exists(test_ecsact_file_path))
		<< "Cannot find test ecsact file: "
		<< fs::absolute(test_ecsact_file_path).string();

#if _WIN32
	// TODO: this doesn't work on linux
	auto exit_code = codegen_command(std::vector{
		"ecsact"s,
		"codegen"s,
		std::string{test_ecsact_file_path},
		std::format("--plugin={}", test_codegen_plugin_path),
		"--outdir=_test_codegen_outdir"s,
	});
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

	ASSERT_TRUE(fs::exists(generated_txt_file_path));
	auto generated_txt_file = std::ifstream{generated_txt_file_path};
	auto generated_txt_file_contents = std::string{
		std::istreambuf_iterator<char>(generated_txt_file),
		std::istreambuf_iterator<char>(),
	};

	ASSERT_EQ(generated_txt_file_contents, "throw this in the text file")
		<< "Unexpected content at "
		<< fs::absolute(generated_txt_file_path).generic_string();

	ASSERT_TRUE(fs::exists(generated_zomsky_file_path));
	auto generated_zomsky_file = std::ifstream{generated_zomsky_file_path};
	auto generated_zomsky_file_contents = std::string{
		std::istreambuf_iterator<char>(generated_zomsky_file),
		std::istreambuf_iterator<char>(),
	};

	ASSERT_EQ(generated_zomsky_file_contents, "throw this in the zomsky file")
		<< "Unexpected content at "
		<< fs::absolute(generated_zomsky_file_path).generic_string();
}
