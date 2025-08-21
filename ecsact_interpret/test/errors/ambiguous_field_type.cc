#include "gtest/gtest.h"
#include "ecsact/interpret/eval.h"

#include "test_lib.hh"

TEST(EvalError, AmbiguousFieldType) {
	auto errs = ecsact_interpret_test_files({
		"errors/ambiguous_field_type.ecsact",
		"errors/ambiguous_field_type_import.ecsact",
	});
	ASSERT_EQ(errs.size(), 1);
	ASSERT_EQ(errs[0].eval_error, ECSACT_EVAL_ERR_AMBIGUOUS_FIELD_TYPE);
}
