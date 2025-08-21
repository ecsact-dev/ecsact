#include "gtest/gtest.h"

#include <string_view>
#include <string>
#include "ecsact/parse.h"

using namespace std::string_literals;

TEST(Parse, RequiredEntityConstraint) {
	ecsact_statement generates_statement{
		.type = ECSACT_STATEMENT_SYSTEM_GENERATES,
		.data{},
	};

	auto                statement_str = "optional ExampleComponent;"s;
	ecsact_statement    statement;
	ecsact_parse_status status;

	auto read_amount = ecsact_parse_statement(
		statement_str.data(),
		statement_str.size(),
		&generates_statement,
		&statement,
		&status
	);

	ASSERT_EQ(statement.type, ECSACT_STATEMENT_ENTITY_CONSTRAINT);
	EXPECT_EQ(status.code, ECSACT_PARSE_STATUS_OK);

	EXPECT_TRUE(statement.data.entity_constraint_statement.optional);
	EXPECT_EQ(
		"ExampleComponent",
		std::string_view(
			statement.data.entity_constraint_statement.constraint_component_name.data,
			statement.data.entity_constraint_statement.constraint_component_name
				.length
		)
	);

	EXPECT_EQ(read_amount, statement_str.size());
}

TEST(Parse, OptionalEntityConstraint) {
	ecsact_statement generates_statement{
		.type = ECSACT_STATEMENT_SYSTEM_GENERATES,
		.data{},
	};

	auto                statement_str = "required ExampleComponent;"s;
	ecsact_statement    statement;
	ecsact_parse_status status;

	auto read_amount = ecsact_parse_statement(
		statement_str.data(),
		statement_str.size(),
		&generates_statement,
		&statement,
		&status
	);

	ASSERT_EQ(statement.type, ECSACT_STATEMENT_ENTITY_CONSTRAINT);
	EXPECT_EQ(status.code, ECSACT_PARSE_STATUS_OK);

	EXPECT_FALSE(statement.data.entity_constraint_statement.optional);
	EXPECT_EQ(
		"ExampleComponent",
		std::string_view(
			statement.data.entity_constraint_statement.constraint_component_name.data,
			statement.data.entity_constraint_statement.constraint_component_name
				.length
		)
	);

	EXPECT_EQ(read_amount, statement_str.size());
}
