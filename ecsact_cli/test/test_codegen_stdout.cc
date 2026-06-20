#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <format>
#include <filesystem>
#include <fstream>
#include <boost/process.hpp>
#include <boost/asio.hpp>
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
	auto bad_generated_file_path =
		fs::path{test_ecsact_file_path}.replace_extension("ecsact.txt");
	if(fs::exists(bad_generated_file_path)) {
		fs::remove(bad_generated_file_path);
	}

	auto ioc = boost::asio::io_context{};
	auto proc_stdout = boost::asio::readable_pipe{ioc};
	auto proc = bp::process{
		ioc,
		test_ecsact_cli,
		{
			"codegen"s,
			std::string{test_ecsact_file_path},
			std::format("--plugin={}", test_codegen_plugin_path),
			"--stdout"s,
		},
		bp::process_stdio{nullptr, proc_stdout, nullptr}
	};

	auto proc_stdout_str = std::string{};
	boost::asio::streambuf buffer;
	boost::system::error_code ec;
	boost::asio::read_until(proc_stdout, buffer, '\n', ec);
	std::istream is(&buffer);
	std::getline(is, proc_stdout_str);

	proc.wait();

	auto exit_code = proc.exit_code();
	ASSERT_EQ(exit_code, 0);

	EXPECT_EQ(proc_stdout_str, "this is just a test, breathe!");
	EXPECT_FALSE(fs::exists(bad_generated_file_path))
		<< bad_generated_file_path.string()
		<< " was generated even with --stdout option!";
}
