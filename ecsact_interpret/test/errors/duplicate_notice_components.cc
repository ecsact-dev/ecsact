#include "gtest/gtest.h"
#include "ecsact/interpret/eval.h"

#include "test_lib.hh"

TEST(DuplicateNoticeComponents, DuplicateNoticeComponents) {
	auto errs =
		ecsact_interpret_test_files({"errors/duplicate_notice_components.ecsact"});
	ASSERT_EQ(errs.size(), 1);
	ASSERT_EQ(errs[0].eval_error, ECSACT_EVAL_ERR_DUPLICATE_NOTIFY_COMPONENT);
}
