#include "gtest/gtest.h"

#include <string_view>
#include <string>
#include <stack>
#include "ecsact/parse.h"

using namespace std::string_literals;

static auto statement_str = R"(
component TestComponent;
action TestAction {
	i32 test_field;
	system TestNestedSystem {
		readonly TestComponent;
	}
}
)"s;

TEST(Parse, SequentialStatements) {
	std::stack<ecsact_statement> context_stack;
	ecsact_statement             statement = {};
	ecsact_parse_status          status = {};

	auto data = statement_str.data();
	auto size = statement_str.size();
	int  read_amount = 0;

	auto read_next = [&] {
		ecsact_statement* context_statement = nullptr;
		if(!context_stack.empty()) {
			context_statement = &context_stack.top();
		}

		read_amount = ecsact_parse_statement(
			data,
			size,
			context_statement,
			&statement,
			&status
		);
		data += read_amount;
		size -= read_amount;

		if(status.code == ECSACT_PARSE_STATUS_BLOCK_BEGIN) {
			context_stack.emplace(statement);
		} else if(status.code == ECSACT_PARSE_STATUS_BLOCK_END) {
			context_stack.pop();
		}
	};

	// Reading `component TestComponent;`
	read_next();
	ASSERT_EQ(status.code, ECSACT_PARSE_STATUS_OK);
	ASSERT_EQ(statement.type, ECSACT_STATEMENT_COMPONENT);

	// Reading `action TestAction {`
	read_next();
	ASSERT_EQ(status.code, ECSACT_PARSE_STATUS_BLOCK_BEGIN);
	ASSERT_EQ(statement.type, ECSACT_STATEMENT_ACTION);

	// Reading nested `i32 test_field;`
	read_next();
	ASSERT_EQ(status.code, ECSACT_PARSE_STATUS_OK);
	ASSERT_EQ(statement.type, ECSACT_STATEMENT_BUILTIN_TYPE_FIELD);
	ASSERT_EQ(statement.data.field_statement.field_type, ECSACT_I32);

	// Reading `system TestNestedSystem {`
	read_next();
	ASSERT_EQ(status.code, ECSACT_PARSE_STATUS_BLOCK_BEGIN);
	ASSERT_EQ(statement.type, ECSACT_STATEMENT_SYSTEM);

	// Reading `readonly TestComponent;`
	read_next();
	ASSERT_EQ(status.code, ECSACT_PARSE_STATUS_OK);
	ASSERT_EQ(statement.type, ECSACT_STATEMENT_SYSTEM_COMPONENT);

	// Reading nested block end `}`
	read_next();
	ASSERT_EQ(status.code, ECSACT_PARSE_STATUS_BLOCK_END);
	ASSERT_EQ(statement.type, ECSACT_STATEMENT_NONE);

	// Reading block end `}`
	read_next();
	ASSERT_EQ(status.code, ECSACT_PARSE_STATUS_BLOCK_END);
	ASSERT_EQ(statement.type, ECSACT_STATEMENT_NONE);
}
