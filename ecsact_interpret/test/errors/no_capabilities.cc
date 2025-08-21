#include "gtest/gtest.h"
#include "ecsact/interpret/eval.h"

#include "test_lib.hh"

TEST(NoCapabilities, NoCapabilitiesAction) {
	auto errs = ecsact_interpret_test_files({"errors/no_capabilities.ecsact"});
	ASSERT_EQ(errs.size(), 1);
	ASSERT_EQ(errs[0].eval_error, ECSACT_EVAL_ERR_NO_CAPABILITIES);
}
