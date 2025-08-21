
#include "gtest/gtest.h"

#include <string_view>
#include <string>
#include <vector>
#include <tuple>
#include "ecsact/parse.h"

using namespace std::string_literals;
using namespace std::string_view_literals;

using test_param_expected = std::vector<std::tuple<
	std::string,
	ecsact_statement_parameter_value_type,
	ecsact_statement_parameter_value_data>>;

void test_param(std::string params, test_param_expected expected) {
	auto statement_str = "system TestSystem" + params + ";"s;
	auto statement = ecsact_statement{};
	auto status = ecsact_parse_status{};

	auto read_amount = ecsact_parse_statement(
		statement_str.data(),
		statement_str.size(),
		nullptr,
		&statement,
		&status
	);

	EXPECT_EQ(status.code, ECSACT_PARSE_STATUS_OK);

	ASSERT_EQ(statement.parameters_length, expected.size());

	int i = 0;
	for(auto&& [expected_name, expected_type, expected_value] : expected) {
		EXPECT_EQ(
			expected_name,
			std::string(
				statement.parameters[i].name.data,
				static_cast<size_t>(statement.parameters[i].name.length)
			)
		);
		ASSERT_EQ(expected_type, statement.parameters[i].value.type);
		switch(expected_type) {
			case ECSACT_STATEMENT_PARAM_VALUE_TYPE_BOOL:
				EXPECT_EQ(
					expected_value.bool_value,
					statement.parameters[i].value.data.bool_value
				);
				break;
			case ECSACT_STATEMENT_PARAM_VALUE_TYPE_INTEGER:
				EXPECT_EQ(
					expected_value.integer_value,
					statement.parameters[i].value.data.integer_value
				);
				break;
			case ECSACT_STATEMENT_PARAM_VALUE_TYPE_FLOAT:
				EXPECT_EQ(
					expected_value.float_value,
					statement.parameters[i].value.data.float_value
				);
				break;
			case ECSACT_STATEMENT_PARAM_VALUE_TYPE_STRING:
				EXPECT_EQ(
					std::string_view(
						expected_value.string_value.data,
						static_cast<size_t>(expected_value.string_value.length)
					),
					std::string_view(
						statement.parameters[i].value.data.string_value.data,
						static_cast<size_t>(
							statement.parameters[i].value.data.string_value.length
						)
					)
				);
				break;
		}

		i += 1;
	}

	EXPECT_EQ(read_amount, statement_str.size());
}

TEST(Parse, SingleBoolParam) {
	using namespace std::string_literals;

	test_param(
		"(bparam: true)",
		{
			std::make_tuple(
				"bparam"s,
				ECSACT_STATEMENT_PARAM_VALUE_TYPE_BOOL,
				ecsact_statement_parameter_value_data{.bool_value = true}
			),
		}
	);

	test_param(
		"(cool_parameter_yo: false)",
		{
			std::make_tuple(
				"cool_parameter_yo"s,
				ECSACT_STATEMENT_PARAM_VALUE_TYPE_BOOL,
				ecsact_statement_parameter_value_data{.bool_value = false}
			),
		}
	);

	test_param(
		"(simple)",
		{
			std::make_tuple(
				"simple"s,
				ECSACT_STATEMENT_PARAM_VALUE_TYPE_BOOL,
				ecsact_statement_parameter_value_data{.bool_value = true}
			),
		}
	);
}

TEST(Parse, SingleIntegerParam) {
	using namespace std::string_literals;

	test_param(
		"(lazy: 1)",
		{
			std::make_tuple(
				"lazy"s,
				ECSACT_STATEMENT_PARAM_VALUE_TYPE_INTEGER,
				ecsact_statement_parameter_value_data{.integer_value = 1}
			),
		}
	);

	test_param(
		"(longer_integer_parameter: 42)",
		{
			std::make_tuple(
				"longer_integer_parameter"s,
				ECSACT_STATEMENT_PARAM_VALUE_TYPE_INTEGER,
				ecsact_statement_parameter_value_data{.integer_value = 42}
			),
		}
	);
}

TEST(Parse, MultiParam) {
	using namespace std::string_literals;

	test_param(
		"(a, b,c)",
		{
			std::make_tuple(
				"a"s,
				ECSACT_STATEMENT_PARAM_VALUE_TYPE_BOOL,
				ecsact_statement_parameter_value_data{.bool_value = true}
			),
			std::make_tuple(
				"b"s,
				ECSACT_STATEMENT_PARAM_VALUE_TYPE_BOOL,
				ecsact_statement_parameter_value_data{.bool_value = true}
			),
			std::make_tuple(
				"c"s,
				ECSACT_STATEMENT_PARAM_VALUE_TYPE_BOOL,
				ecsact_statement_parameter_value_data{.bool_value = true}
			),
		}
	);

	test_param(
		"(cool_int: 0, fancy, optimized: false)",
		{
			std::make_tuple(
				"cool_int"s,
				ECSACT_STATEMENT_PARAM_VALUE_TYPE_INTEGER,
				ecsact_statement_parameter_value_data{.integer_value = 0}
			),
			std::make_tuple(
				"fancy"s,
				ECSACT_STATEMENT_PARAM_VALUE_TYPE_BOOL,
				ecsact_statement_parameter_value_data{.bool_value = true}
			),
			std::make_tuple(
				"optimized"s,
				ECSACT_STATEMENT_PARAM_VALUE_TYPE_BOOL,
				ecsact_statement_parameter_value_data{.bool_value = false}
			),
		}
	);

	test_param(
		"(cool_str_param: whatup)",
		{
			std::make_tuple(
				"cool_str_param"s,
				ECSACT_STATEMENT_PARAM_VALUE_TYPE_STRING,
				ecsact_statement_parameter_value_data{
					.string_value{"whatup"sv.data(), "whatup"sv.size()}
				}
			),
		}
	);
}
