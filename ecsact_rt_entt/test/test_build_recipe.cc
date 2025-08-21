#include "gtest/gtest.h"
#include <string>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include <format>

#include <gtest/gtest.h>
#include "spawn.hh"

namespace fs = std::filesystem;

TEST(Build, Success) {
	auto ecsact_cli = std::getenv("ECSACT_CLI");
	auto ecsact_codegen_plugin_path = std::getenv("ECSACT_CODEGEN_PLUGIN_PATH");
	auto ecsact_runtime_file_path = std::getenv("ECSACT_RUNTIME_FILE_PATH");
	auto ecsact_imported_file_path = std::getenv("ECSACT_IMPORTED_FILE_PATH");
	auto ecsact_recipe_path = std::getenv("ECSACT_RECIPE_PATH");

	ASSERT_NE(ecsact_cli, nullptr);
	ASSERT_NE(ecsact_codegen_plugin_path, nullptr);
	ASSERT_NE(ecsact_runtime_file_path, nullptr);
	ASSERT_NE(ecsact_imported_file_path, nullptr);
	ASSERT_NE(ecsact_recipe_path, nullptr);

	ASSERT_TRUE(fs::exists(ecsact_cli));
	ASSERT_TRUE(fs::exists(ecsact_codegen_plugin_path));
	ASSERT_TRUE(fs::exists(ecsact_runtime_file_path));
	ASSERT_TRUE(fs::exists(ecsact_imported_file_path));
	ASSERT_TRUE(fs::exists(ecsact_recipe_path));

	std::vector<std::string> args{
		"build",
		std::string(ecsact_runtime_file_path),
		std::string(ecsact_imported_file_path),
		std::format("--recipe={}", ecsact_recipe_path),
		std::format("--output={}", "test"),
	};

	int exit_code =
		ecsact::entt::test::detail::spawn(std::string(ecsact_cli), args);

	ASSERT_EQ(exit_code, 0);
}
