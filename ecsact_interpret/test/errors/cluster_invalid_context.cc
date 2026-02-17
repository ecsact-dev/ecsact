#include "gtest/gtest.h"
#include <vector>
#include "ecsact/interpret/eval.hh"
#include "bazel/runfiles/runfiles.hh"

TEST(EcsactInterpret, ClusterInvalidContextError) {
	auto runfiles = bazel_sundry::CreateDefaultRunfiles();
	ASSERT_TRUE(runfiles);

	auto test_ecsact = runfiles->Rlocation(
		"ecsact/ecsact_interpret/test/errors/cluster_invalid_context.ecsact"
	);

	auto errors = ecsact::eval_files({test_ecsact});
	ASSERT_EQ(errors.size(), 2);
	EXPECT_EQ(errors[0].eval_error, 0); // parse error
	EXPECT_EQ(errors[0].line, 3);
	EXPECT_EQ(errors[1].eval_error, ECSACT_EVAL_ERR_INVALID_CONTEXT);
	EXPECT_EQ(errors[1].line, 4);
}
