#include "gtest/gtest.h"

#include "ecsact_interpret/test/test_lib.hh"

TEST(InterpretError, UnknownAssociationField) {
	auto errs =
		ecsact_interpret_test_files({"errors/unknown_association_field.ecsact"});
	ASSERT_EQ(errs.size(), 1);
	ASSERT_EQ(errs[0].eval_error, ECSACT_EVAL_ERR_UNKNOWN_FIELD_NAME);
}
