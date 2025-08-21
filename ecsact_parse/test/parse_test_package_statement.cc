#include "gtest/gtest.h"

#include <string_view>
#include <string>
#include "ecsact/parse.h"

using namespace std::string_literals;

TEST(Parse, Package) {
	auto                statement_str = "package example;"s;
	ecsact_statement    statement;
	ecsact_parse_status status;

	auto read_amount = ecsact_parse_statement(
		statement_str.data(),
		statement_str.size(),
		nullptr,
		&statement,
		&status
	);

	ASSERT_EQ(statement.type, ECSACT_STATEMENT_PACKAGE);
	EXPECT_EQ(status.code, ECSACT_PARSE_STATUS_OK);

	EXPECT_FALSE(statement.data.package_statement.main);
	EXPECT_EQ(
		"example",
		std::string_view(
			statement.data.package_statement.package_name.data,
			statement.data.package_statement.package_name.length
		)
	);

	EXPECT_EQ(read_amount, statement_str.size());
}

TEST(Parse, MainPackage) {
	auto                statement_str = "main package example;"s;
	ecsact_statement    statement;
	ecsact_parse_status status;

	auto read_amount = ecsact_parse_statement(
		statement_str.data(),
		statement_str.size(),
		nullptr,
		&statement,
		&status
	);

	ASSERT_EQ(statement.type, ECSACT_STATEMENT_PACKAGE);
	EXPECT_EQ(status.code, ECSACT_PARSE_STATUS_OK);

	EXPECT_TRUE(statement.data.package_statement.main);
	EXPECT_EQ(
		"example",
		std::string_view(
			statement.data.package_statement.package_name.data,
			statement.data.package_statement.package_name.length
		)
	);

	EXPECT_EQ(read_amount, statement_str.size());
}
