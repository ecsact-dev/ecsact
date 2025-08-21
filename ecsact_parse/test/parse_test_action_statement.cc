#include "gtest/gtest.h"

#include <string_view>
#include <string>
#include "ecsact/parse.h"

using namespace std::string_literals;

TEST(Parse, Action) {
	auto                statement_str = "action MyAction;"s;
	ecsact_statement    statement;
	ecsact_parse_status status;

	auto read_amount = ecsact_parse_statement(
		statement_str.data(),
		statement_str.size(),
		nullptr,
		&statement,
		&status
	);

	ASSERT_EQ(statement.type, ECSACT_STATEMENT_ACTION);
	EXPECT_EQ(status.code, ECSACT_PARSE_STATUS_OK);

	EXPECT_EQ(
		"MyAction",
		std::string_view(
			statement.data.action_statement.action_name.data,
			statement.data.action_statement.action_name.length
		)
	);

	EXPECT_EQ(read_amount, statement_str.size());
}
