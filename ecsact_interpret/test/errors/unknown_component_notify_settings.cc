#include "gtest/gtest.h"
#include "ecsact/interpret/eval.h"

#include "test_lib.hh"

TEST(UnknownComponentNotifySettings, UnknownComponentNotifySettings) {
	auto errs =
		ecsact_interpret_test_files({"errors/"
																 "unknown_component_notify_settings.ecsact"});
	ASSERT_EQ(errs.size(), 1);
	ASSERT_EQ(errs[0].eval_error, ECSACT_EVAL_ERR_UNKNOWN_COMPONENT_LIKE_TYPE);
}
