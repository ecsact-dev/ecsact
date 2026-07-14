#include "gtest/gtest.h"

#include <string_view>
#include <string>
#include "ecsact/parse.h"

using namespace std::string_literals;

TEST(Parse, MetadataDummy) {
	SUCCEED();
}

#if 0
TEST(Parse, MetadataStatement) {
	auto                statement_str = "metadata;"s;
	ecsact_statement    statement;
	ecsact_parse_status status;

	auto read_amount = ecsact_parse_statement(
		statement_str.data(),
		statement_str.size(),
		nullptr,
		&statement,
		&status
	);

	ASSERT_EQ(statement.type, ECSACT_STATEMENT_METADATA);
	EXPECT_EQ(status.code, ECSACT_PARSE_STATUS_OK);
	EXPECT_EQ(read_amount, statement_str.size());
}

TEST(Parse, OpaqueFieldStatement) {
	auto             parent_name = "metadata"s;
	ecsact_statement parent_statement{
		.type = ECSACT_STATEMENT_METADATA,
	};

	auto                statement_str = "opaque my_ptr_field;"s;
	ecsact_statement    statement;
	ecsact_parse_status status;

	auto read_amount = ecsact_parse_statement(
		statement_str.data(),
		statement_str.size(),
		&parent_statement,
		&statement,
		&status
	);

	ASSERT_EQ(statement.type, ECSACT_STATEMENT_BUILTIN_TYPE_FIELD);
	EXPECT_EQ(status.code, ECSACT_PARSE_STATUS_OK);
	EXPECT_EQ(read_amount, statement_str.size());
	EXPECT_EQ(statement.data.field_statement.field_type, ECSACT_OPAQUE);
	EXPECT_EQ(
		"my_ptr_field",
		std::string_view(
			statement.data.field_statement.field_name.data,
			statement.data.field_statement.field_name.length
		)
	);
}
#endif
