#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <format>
#include <ranges>
#include <filesystem>
#include <fstream>
#include <boost/process.hpp>
#include "tools/cpp/runfiles/runfiles.h"

using bazel::tools::cpp::runfiles::Runfiles;
using namespace std::string_literals;
namespace fs = std::filesystem;
namespace bp = boost::process;

TEST(Codegen, Stdout) {
	auto runfiles_err = std::string{};
	auto runfiles = Runfiles::CreateForTest(&runfiles_err);
	auto test_ecsact_cli = std::getenv("TEST_ECSACT_CLI");
	auto test_codegen_plugin_path = std::getenv("TEST_CODEGEN_PLUGIN_PATH");
	auto test_ecsact_file_path = std::getenv("TEST_ECSACT_FILE_PATH");

	ASSERT_NE(test_ecsact_cli, nullptr);
	ASSERT_NE(test_codegen_plugin_path, nullptr);
	ASSERT_NE(test_ecsact_file_path, nullptr);
	ASSERT_NE(runfiles, nullptr) << runfiles_err;
	ASSERT_TRUE(fs::exists(test_codegen_plugin_path));
	ASSERT_TRUE(fs::exists(test_ecsact_file_path));

	// this file should NOT be generated
	auto bad_generated_file_path = fs::path{"test.txt"s};
	if(fs::exists(bad_generated_file_path)) {
		fs::remove(bad_generated_file_path);
	}

	auto proc_stdout = bp::ipstream{};
	auto proc = bp::child{
		bp::exe(test_ecsact_cli),
		bp::args({
			"codegen"s,
			std::string{test_ecsact_file_path},
			std::format("--plugin={}", test_codegen_plugin_path),
			"--print-output-files"s,
		}),
		bp::std_out > proc_stdout
	};

	proc.wait();

	auto exit_code = proc.exit_code();
	ASSERT_EQ(exit_code, 0);

	auto output_files = std::vector<std::string>{};
	auto line = std::string{};
	while(std::getline(proc_stdout, line)) {
		if(line.ends_with("\r")) {
			line.pop_back();
		}
		output_files.emplace_back(line);
	}

	EXPECT_EQ(output_files.size(), 2);

	EXPECT_FALSE(
		std::ranges::find(output_files, "test.txt") == output_files.end()
	);
	EXPECT_FALSE(
		std::ranges::find(output_files, "test.zomsky") == output_files.end()
	);
	EXPECT_FALSE(fs::exists(bad_generated_file_path))
		<< bad_generated_file_path.string()
		<< " was generated even with --print-output-files option!";
}
