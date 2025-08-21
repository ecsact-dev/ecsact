#include "gtest/gtest.h"

#include <string_view>
#include <string>
#include "ecsact/parse.h"

using namespace std::string_literals;

TEST(Parse, SystemGenerates) {
	auto             system_name = "ExampleSystem"s;
	ecsact_statement component_statement{
		.type = ECSACT_STATEMENT_SYSTEM,
		.data{.system_statement{.system_name{
			.data = system_name.data(),
			.length = static_cast<int>(system_name.size()),
		}}},
	};

	auto                statement_str = "generates {"s;
	ecsact_statement    statement;
	ecsact_parse_status status;

	auto read_amount = ecsact_parse_statement(
		statement_str.data(),
		statement_str.size(),
		&component_statement,
		&statement,
		&status
	);

	ASSERT_EQ(statement.type, ECSACT_STATEMENT_SYSTEM_GENERATES);
	EXPECT_EQ(status.code, ECSACT_PARSE_STATUS_BLOCK_BEGIN);
	EXPECT_EQ(read_amount, statement_str.size());
}
