#include "gtest/gtest.h"

#include <string_view>
#include <string>
#include "ecsact/parse.h"

using namespace std::string_literals;

TEST(Parse, NoneStatement) {
	auto                statement_str = "asd asd;"s;
	ecsact_statement    statement;
	ecsact_parse_status status;

	auto read_amount = ecsact_parse_statement(
		statement_str.data(),
		statement_str.size(),
		nullptr,
		&statement,
		&status
	);

	ASSERT_EQ(status.code, ECSACT_PARSE_STATUS_OK);
	ASSERT_EQ(statement.type, ECSACT_STATEMENT_UNKNOWN);
	EXPECT_EQ(read_amount, statement_str.size());
}
