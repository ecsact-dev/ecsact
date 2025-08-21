#include "gtest/gtest.h"
#include <iostream>
#include <ranges>
#include <cassert>
#include "ecsact/cli/commands/build/build_recipe.hh"

using ecsact::build_recipe;
using namespace std::string_literals;

constexpr auto RECIPE_A = R"yaml(
name: A
sources:
- xilo/yama/zompers.cc

imports: []
exports: [ecsact_create_registry]
)yaml";

constexpr auto RECIPE_B = R"yaml(
name: B
sources:
- bad/beaver.cc

imports: []
exports: [ecsact_destroy_registry]
)yaml";

constexpr auto RECIPE_C = R"yaml(
name: C
sources:
- xilo/yama/vedder/dog.cc

imports: []
exports: [ecsact_clear_registry]
)yaml";

constexpr auto RECIPE_D = R"yaml(
name: D
sources:
- d.cc

imports: []
exports: [ecsact_clear_registry]
)yaml";

constexpr auto RECIPE_E = R"yaml(
name: E
sources:
- e.cc

imports: []
exports: [ecsact_destroy_registry]
)yaml";

auto contains_source_path(auto&& r, auto p) -> bool {
	using std::ranges::find_if;

	auto itr = find_if(r, [&](auto&& src) -> bool {
		auto src_path = std::get_if<build_recipe::source_path>(&src);
		if(!src_path) {
			return false;
		}

		return src_path->path.generic_string() == p;
	});

	return itr != r.end();
}

auto sources_path_str(auto&& r) -> std::string {
	auto str = std::string{};

	for(auto&& src : r) {
		auto src_path = std::get_if<build_recipe::source_path>(&src);
		if(!src_path) {
			continue;
		}
		str += "  " + src_path->path.generic_string() + "\n";
	}

	return str;
}

TEST(RecipeMerge, Correct) {
	auto a = std::get<build_recipe>(build_recipe::from_yaml_string( //
		RECIPE_A,
		"zeke/apple.yml"
	));
	auto b = std::get<build_recipe>(build_recipe::from_yaml_string( //
		RECIPE_B,
		"ban.yml"
	));
	auto c = std::get<build_recipe>(build_recipe::from_yaml_string( //
		RECIPE_C,
		"zeke/bonbon/quit/cover.yml"
	));

	auto ab_m = std::get<build_recipe>(build_recipe::merge(a, b));

	EXPECT_EQ(ab_m.base_directory(), a.base_directory());

	EXPECT_TRUE(contains_source_path(ab_m.sources(), "xilo/yama/zompers.cc"))
		<< "Found:\n"
		<< sources_path_str(ab_m.sources());

	EXPECT_TRUE(contains_source_path(ab_m.sources(), "../bad/beaver.cc"))
		<< "Found:\n"
		<< sources_path_str(ab_m.sources());

	auto abc_m = std::get<build_recipe>(build_recipe::merge(ab_m, c));

	EXPECT_EQ(abc_m.base_directory(), ab_m.base_directory());

	EXPECT_TRUE(contains_source_path(abc_m.sources(), "xilo/yama/zompers.cc"))
		<< "Found:\n"
		<< sources_path_str(abc_m.sources());

	EXPECT_TRUE(contains_source_path(abc_m.sources(), "../bad/beaver.cc"))
		<< "Found:\n"
		<< sources_path_str(abc_m.sources());

	EXPECT_TRUE(
		contains_source_path(abc_m.sources(), "bonbon/quit/xilo/yama/vedder/dog.cc")
	) << "Found:\n"
		<< sources_path_str(abc_m.sources());
}

TEST(RecipeMerge, Correct2) {
	auto d = std::get<build_recipe>(build_recipe::from_yaml_string( //
		RECIPE_D,
		"job/d.yml"
	));
	auto e = std::get<build_recipe>(build_recipe::from_yaml_string( //
		RECIPE_E,
		"job/e.yml"
	));

	auto de_m = std::get<build_recipe>(build_recipe::merge(d, e));

	EXPECT_EQ(d.base_directory(), de_m.base_directory());

	EXPECT_TRUE(contains_source_path(de_m.sources(), "d.cc"))
		<< "Found:\n"
		<< sources_path_str(de_m.sources());

	EXPECT_TRUE(contains_source_path(de_m.sources(), "e.cc"))
		<< "Found:\n"
		<< sources_path_str(de_m.sources());
}
