#include "gtest/gtest.h"

#include <string_view>
#include <string>
#include "ecsact/parse.h"

using namespace std::string_literals;

TEST(Parse, NoneStatement) {
	auto                statement_str = "  "s;
	ecsact_statement    statement;
	ecsact_parse_status status;

	auto read_amount = ecsact_parse_statement(
		statement_str.data(),
		statement_str.size(),
		nullptr,
		&statement,
		&status
	);

	ASSERT_FALSE(ecsact_is_error_parse_status_code(status.code));
	EXPECT_EQ(status.code, ECSACT_PARSE_STATUS_ASSUMED_STATEMENT_END);
	EXPECT_EQ(statement.type, ECSACT_STATEMENT_NONE);
	EXPECT_EQ(read_amount, statement_str.size());
}
