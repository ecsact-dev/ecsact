#include "gtest/gtest.h"

#include <string_view>
#include <string>
#include <ranges>
#include <array>
#include <concepts>
#include "ecsact/parse.h"

using namespace std::string_literals;
using namespace std::string_view_literals;

template<typename R, typename V>
concept RangeOf =
	std::ranges::range<R> && std::same_as<std::ranges::range_value_t<R>, V>;

constexpr auto empty_list = std::array<std::string_view, 0>{};

void TestValidSystemComponent(
	std::string_view               statement_str,
	ecsact_system_capability       expected_capability,
	std::string_view               expected_component_name,
	RangeOf<std::string_view> auto expected_with_field_names = empty_list
) {
	auto system_name = "ExampleSystem"s;
	auto context_statement = ecsact_statement{
		.type = ECSACT_STATEMENT_SYSTEM,
		.data{.system_statement{.system_name{
			.data = system_name.data(),
			.length = static_cast<int>(system_name.size()),
		}}},
	};

	ecsact_statement    statement;
	ecsact_parse_status status;

	auto read_amount = ecsact_parse_statement(
		statement_str.data(),
		statement_str.size(),
		&context_statement,
		&statement,
		&status
	);

	ASSERT_EQ(status.code, ECSACT_PARSE_STATUS_OK);
	ASSERT_EQ(statement.type, ECSACT_STATEMENT_SYSTEM_COMPONENT);

	EXPECT_EQ(
		expected_capability,
		statement.data.system_component_statement.capability
	);
	EXPECT_EQ(
		expected_component_name,
		std::string_view(
			statement.data.system_component_statement.component_name.data,
			statement.data.system_component_statement.component_name.length
		)
	);

	ASSERT_EQ(
		statement.data.system_component_statement.with_field_name_list_count,
		expected_with_field_names.size()
	);

	for(int i = 0; std::size(expected_with_field_names) > i; ++i) {
		EXPECT_EQ(
			expected_with_field_names[i],
			std::string_view(
				statement.data.system_component_statement.with_field_name_list[i].data,
				statement.data.system_component_statement.with_field_name_list[i].length
			)
		);
	}

	EXPECT_EQ(read_amount, statement_str.size());
}

TEST(Parse, ReadonlySystemComponent) {
	TestValidSystemComponent(
		"readonly ExampleComponent;"s,
		ECSACT_SYS_CAP_READONLY,
		"ExampleComponent",
		empty_list
	);
}

TEST(Parse, WriteonlySystemComponent) {
	TestValidSystemComponent(
		"writeonly ExampleComponent;"s,
		ECSACT_SYS_CAP_WRITEONLY,
		"ExampleComponent",
		empty_list
	);
}

TEST(Parse, ReadwriteSystemComponent) {
	TestValidSystemComponent(
		"readwrite ExampleComponent;"s,
		ECSACT_SYS_CAP_READWRITE,
		"ExampleComponent",
		empty_list
	);
}

TEST(Parse, OptionalReadonlySystemComponent) {
	TestValidSystemComponent(
		"optional readonly ExampleComponent;"s,
		ECSACT_SYS_CAP_OPTIONAL_READONLY,
		"ExampleComponent",
		empty_list
	);
}

TEST(Parse, OptionalWriteonlySystemComponent) {
	TestValidSystemComponent(
		"optional writeonly ExampleComponent;"s,
		ECSACT_SYS_CAP_OPTIONAL_WRITEONLY,
		"ExampleComponent",
		empty_list
	);
}

TEST(Parse, OptionalReadwriteSystemComponent) {
	TestValidSystemComponent(
		"optional readwrite ExampleComponent;"s,
		ECSACT_SYS_CAP_OPTIONAL_READWRITE,
		"ExampleComponent",
		empty_list
	);
}

TEST(Parse, IncludeSystemComponent) {
	TestValidSystemComponent(
		"include ExampleComponent;"s,
		ECSACT_SYS_CAP_INCLUDE,
		"ExampleComponent",
		empty_list
	);
}

TEST(Parse, ExcludeSystemComponent) {
	TestValidSystemComponent(
		"exclude ExampleComponent;"s,
		ECSACT_SYS_CAP_EXCLUDE,
		"ExampleComponent",
		empty_list
	);
}

TEST(Parse, AddsSystemComponent) {
	TestValidSystemComponent(
		"adds ExampleComponent;"s,
		ECSACT_SYS_CAP_ADDS,
		"ExampleComponent",
		empty_list
	);
}

TEST(Parse, RemovesSystemComponent) {
	TestValidSystemComponent(
		"removes ExampleComponent;"s,
		ECSACT_SYS_CAP_REMOVES,
		"ExampleComponent",
		empty_list
	);
}

TEST(Parse, StreamToggleSystemComponent) {
	TestValidSystemComponent(
		"stream_toggle ExampleComponent;"s,
		ECSACT_SYS_CAP_STREAM_TOGGLE,
		"ExampleComponent",
		empty_list
	);
}

///////////////////////////////////
// 'with' keyword                //
///////////////////////////////////

TEST(Parse, ReadonlySystemComponentWithEntity) {
	TestValidSystemComponent(
		"readonly ExampleComponent with example_entity;"s,
		ECSACT_SYS_CAP_READONLY,
		"ExampleComponent",
		std::array{"example_entity"sv}
	);
}

TEST(Parse, WriteonlySystemComponentWithEntity) {
	TestValidSystemComponent(
		"writeonly ExampleComponent with example_entity;"s,
		ECSACT_SYS_CAP_WRITEONLY,
		"ExampleComponent",
		std::array{"example_entity"sv}
	);
}

TEST(Parse, ReadwriteSystemComponentWithEntity) {
	TestValidSystemComponent(
		"readwrite ExampleComponent with example_entity;"s,
		ECSACT_SYS_CAP_READWRITE,
		"ExampleComponent",
		std::array{"example_entity"sv}
	);
}

TEST(Parse, OptionalReadonlySystemComponentWithEntity) {
	TestValidSystemComponent(
		"optional readonly ExampleComponent with example_entity;"s,
		ECSACT_SYS_CAP_OPTIONAL_READONLY,
		"ExampleComponent",
		std::array{"example_entity"sv}
	);
}

TEST(Parse, OptionalWriteonlySystemComponentWithEntity) {
	TestValidSystemComponent(
		"optional writeonly ExampleComponent with example_entity;"s,
		ECSACT_SYS_CAP_OPTIONAL_WRITEONLY,
		"ExampleComponent",
		std::array{"example_entity"sv}
	);
}

TEST(Parse, OptionalReadwriteSystemComponentWithEntity) {
	TestValidSystemComponent(
		"optional readwrite ExampleComponent with example_entity;"s,
		ECSACT_SYS_CAP_OPTIONAL_READWRITE,
		"ExampleComponent",
		std::array{"example_entity"sv}
	);
}

TEST(Parse, IncludeSystemComponentWithEntity) {
	TestValidSystemComponent(
		"include ExampleComponent with example_entity;"s,
		ECSACT_SYS_CAP_INCLUDE,
		"ExampleComponent",
		std::array{"example_entity"sv}
	);
}

////////////////////////////////////
// components from other packages //
////////////////////////////////////

TEST(Parse, OtherPackageFullyQualified) {
	TestValidSystemComponent(
		"readwrite other.pkg.ExampleComponent;"s,
		ECSACT_SYS_CAP_READWRITE,
		"other.pkg.ExampleComponent",
		empty_list
	);
}

TEST(Parse, IncludeSystemComponentWithEntityFullQualified) {
	TestValidSystemComponent(
		"include other.pkg.ExampleComponent with example_entity;"s,
		ECSACT_SYS_CAP_INCLUDE,
		"other.pkg.ExampleComponent",
		std::array{"example_entity"sv}
	);
}
