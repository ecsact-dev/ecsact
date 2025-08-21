#include "gtest/gtest.h"

#include <string_view>
#include <string>
#include "magic_enum.hpp"
#include "ecsact/parse.h"

using namespace std::string_literals;

TEST(Parse, WithStatement) {
	ecsact_statement context;
	context.type = ECSACT_STATEMENT_SYSTEM_COMPONENT;
	context.data.system_component_statement = {};

	auto                statement_str = "with test_field;"s;
	ecsact_statement    statement;
	ecsact_parse_status status;

	auto read_amount = ecsact_parse_statement(
		statement_str.data(),
		statement_str.size(),
		&context,
		&statement,
		&status
	);

	ASSERT_EQ(status.code, ECSACT_PARSE_STATUS_OK);
	ASSERT_EQ(statement.type, ECSACT_STATEMENT_SYSTEM_WITH);

	ASSERT_EQ(statement.data.system_with_statement.with_field_name_list_count, 1);
	EXPECT_EQ(
		"test_field",
		std::string_view(
			statement.data.system_with_statement.with_field_name_list[0].data,
			statement.data.system_with_statement.with_field_name_list[0].length
		)
	);

	EXPECT_EQ(read_amount, statement_str.size());
}

TEST(Parse, WithStatementMultiField) {
	ecsact_statement context;
	context.type = ECSACT_STATEMENT_SYSTEM_COMPONENT;
	context.data.system_component_statement = {};

	auto                statement_str = "with x,y,z;"s;
	ecsact_statement    statement;
	ecsact_parse_status status;

	auto read_amount = ecsact_parse_statement(
		statement_str.data(),
		statement_str.size(),
		&context,
		&statement,
		&status
	);

	ASSERT_EQ(status.code, ECSACT_PARSE_STATUS_OK);
	ASSERT_EQ(statement.type, ECSACT_STATEMENT_SYSTEM_WITH);

	auto& data = statement.data.system_with_statement;

	ASSERT_EQ(data.with_field_name_list_count, 3);

	auto field_name = [&data](auto index) -> std::string_view {
		return std::string_view{
			data.with_field_name_list[index].data,
			static_cast<size_t>(data.with_field_name_list[index].length)
		};
	};

	EXPECT_EQ("x", field_name(0));
	EXPECT_EQ("y", field_name(1));
	EXPECT_EQ("z", field_name(2));

	EXPECT_EQ(read_amount, statement_str.size());
}
