#include "gtest/gtest.h"
#include "ecsact/interpret/eval.h"

#include "test_lib.hh"

TEST(UnknownParamValue, UnknownStreamComponentParamValue) {
	auto errs = ecsact_interpret_test_files({
		"errors/invalid_stream_component_param_value.ecsact",
	});
	ASSERT_EQ(errs.size(), 1);
	ASSERT_EQ(errs[0].eval_error, ECSACT_EVAL_ERR_INVALID_PARAMETER_VALUE);
}
