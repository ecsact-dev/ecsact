
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <format>
#include <filesystem>
#include <fstream>
#include "ecsact/cli/commands/build.hh"
#include "ecsact/cli/commands/recipe-bundle.hh"

using ecsact::cli::detail::build_command;
using ecsact::cli::detail::recipe_bundle_command;

using namespace std::string_literals;
namespace fs = std::filesystem;

TEST(Build, Success) {
	auto test_ecsact_cli = std::getenv("TEST_ECSACT_CLI");
	auto test_codegen_plugin_path = std::getenv("TEST_CODEGEN_PLUGIN_PATH");
	auto test_ecsact_file_path = std::getenv("TEST_ECSACT_FILE_PATH");
	auto test_build_recipe_path = std::getenv("TEST_ECSACT_BUILD_RECIPE_PATH");
	auto test_build_merge_recipe_path =
		std::getenv("TEST_ECSACT_BUILD_MERGE_RECIPE_PATH");

	ASSERT_NE(test_ecsact_cli, nullptr);
	ASSERT_NE(test_codegen_plugin_path, nullptr);
	ASSERT_NE(test_ecsact_file_path, nullptr);
	ASSERT_NE(test_build_recipe_path, nullptr);
	ASSERT_NE(test_build_merge_recipe_path, nullptr);

	ASSERT_TRUE(fs::exists(test_codegen_plugin_path));
	ASSERT_TRUE(fs::exists(test_ecsact_file_path));
	ASSERT_TRUE(fs::exists(test_build_recipe_path));

	auto exit_code = recipe_bundle_command(std::vector{
		"ecsact"s,
		"recipe-bundle"s,
		std::string{test_build_recipe_path},
		std::string{test_build_merge_recipe_path},
		"--output=test"s,
	});

	ASSERT_EQ(exit_code, 0);

	exit_code = build_command(std::vector{
		"ecsact"s,
		"build"s,
		std::string{test_ecsact_file_path},
		"--recipe=test"s,
		"--output=test_bundle_runtime"s,
		"--temp_dir=_test_bundle_runtime_temp"s,
	});

	ASSERT_EQ(exit_code, 0);
}
