#include <gtest/gtest.h>

#include <filesystem>
#include "ecsact/cli/detail/glob.hh"
#include "tools/cpp/runfiles/runfiles.h"

using bazel::tools::cpp::runfiles::Runfiles;
using ecsact::cli::detail::expand_path_globs;
using ecsact::cli::detail::path_before_glob;
using ecsact::cli::detail::path_matches_glob;
using std::ranges::find;

namespace fs = std::filesystem;

class Glob : public testing::Test {
protected:
	fs::path test_root_dir;

	void SetUp() override {
		auto runfiles_err = std::string{};
		auto runfiles = Runfiles::CreateForTest(&runfiles_err);
		ASSERT_NE(runfiles, nullptr) << runfiles_err;
		ASSERT_NE(std::getenv("GLOB_TEST_ROOT_FILE"), nullptr);
		auto readme = fs::path{std::getenv("GLOB_TEST_ROOT_FILE")};
		ASSERT_FALSE(readme.empty());
		ASSERT_TRUE(fs::exists(readme)) << readme.generic_string() << "\n";
		test_root_dir = readme.parent_path();
		if(fs::is_symlink(test_root_dir)) {
			test_root_dir = fs::read_symlink(test_root_dir);
		}
	}
};

TEST_F(Glob, SimpleWildcard) {
	auto glob_pattern = test_root_dir / "*";
	auto ec = std::error_code{};
	auto paths = expand_path_globs(glob_pattern, ec);
	ASSERT_FALSE(ec) << ec.message() << "\n";
	auto expected_paths = std::vector{
		test_root_dir / "README.md",
		test_root_dir / "something",
		test_root_dir / "test",
		test_root_dir / "test.txt",
		test_root_dir / "test2.txt",
	};

	for(auto p : paths) {
		EXPECT_TRUE(find(expected_paths, p) != expected_paths.end())
			<< "Unexpected path: " << p.generic_string() << "\n"
			<< glob_pattern.generic_string() << "\n";
	}
}

TEST_F(Glob, WildcardWithSuffix) {
	auto glob_pattern = test_root_dir / "*.txt";
	auto ec = std::error_code{};
	auto paths = expand_path_globs(glob_pattern, ec);
	ASSERT_FALSE(ec) << ec.message() << "\n";
	auto expected_paths = std::vector{
		test_root_dir / "test.txt",
		test_root_dir / "test2.txt",
	};

	for(auto p : paths) {
		EXPECT_TRUE(find(expected_paths, p) != expected_paths.end())
			<< "Unexpected path: " << p.generic_string() << "\n"
			<< "Glob pattern: " << glob_pattern.generic_string() << "\n";
	}
}

TEST_F(Glob, SimpleRecursiveWildcard) {
	auto glob_pattern = test_root_dir / "**";
	auto ec = std::error_code{};
	auto paths = expand_path_globs(glob_pattern, ec);
	ASSERT_FALSE(ec) << ec.message() << "\n";
	auto expected_paths = std::vector{
		test_root_dir / "README.md",
		test_root_dir / "something",
		test_root_dir / "test",
		test_root_dir / "test.txt",
		test_root_dir / "test2.txt",
		test_root_dir / "README.md",
		test_root_dir / "something",
		test_root_dir / "something" / "other.txt",
		test_root_dir / "test",
		test_root_dir / "test" / "cant_believe.butter",
		test_root_dir / "test" / "can_believe.nothing",
		test_root_dir / "test" / "wow",
		test_root_dir / "test" / "wow" / "so",
		test_root_dir / "test" / "wow" / "so" / "cool.beans",
		test_root_dir / "test" / "wow" / "wow-something.txt",
		test_root_dir / "test.txt",
		test_root_dir / "test2.txt",
	};

	for(auto p : paths) {
		EXPECT_TRUE(find(expected_paths, p) != expected_paths.end())
			<< "Unexpected path: " << p.generic_string() << "\n"
			<< glob_pattern.generic_string() << "\n";
	}
}

TEST_F(Glob, RecursiveWildcardSuffix) {
	auto glob_pattern = test_root_dir / "**.txt";
	auto ec = std::error_code{};
	auto paths = expand_path_globs(glob_pattern, ec);
	ASSERT_FALSE(ec) << ec.message() << "\n";
	auto expected_paths = std::vector{
		test_root_dir / "test.txt",
		test_root_dir / "test2.txt",
		test_root_dir / "something" / "other.txt",
		test_root_dir / "test" / "wow" / "wow-something.txt",
		test_root_dir / "test.txt",
		test_root_dir / "test2.txt",
	};

	for(auto p : paths) {
		EXPECT_TRUE(find(expected_paths, p) != expected_paths.end())
			<< "Unexpected path: " << p.generic_string() << "\n"
			<< glob_pattern.generic_string() << "\n";
	}
}

TEST_F(Glob, PathBeforeGlob) {
	ASSERT_EQ(path_before_glob("a/b/*").generic_string(), "a/b");
	ASSERT_EQ(path_before_glob("a/b/*.txt/c/*.d").generic_string(), "a/b");
	ASSERT_EQ(path_before_glob("a/b/c.txt").generic_string(), "a/b/c.txt");
}

TEST_F(Glob, GlobMatch) {
	EXPECT_TRUE(path_matches_glob("a/b/c", "a/b/*"));
	EXPECT_TRUE(path_matches_glob("a/b/c.txt", "a/b/*.txt"));
	EXPECT_TRUE(path_matches_glob("a/b/c", "a/b/**"));
	EXPECT_TRUE(path_matches_glob("a/b/c/d/e", "a/b/**"));
	EXPECT_TRUE(path_matches_glob("a/b/c/d/e.txt", "a/b/**.txt"));
	EXPECT_TRUE(path_matches_glob("a/b/c", "**"));

	EXPECT_FALSE(path_matches_glob("a/z/c", "a/b/*"));
	EXPECT_FALSE(path_matches_glob("z/b/c/d/e.txt", "a/b/**.txt"));
}
