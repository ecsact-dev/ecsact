#include "gtest/gtest.h"
#include <vector>
#include <iostream>
#include "ecsact/interpret/eval.hh"
#include "bazel/runfiles/runfiles.hh"

TEST(EcsactInterpret, ClusterInvalidConflictError) {
	auto runfiles = bazel_sundry::CreateDefaultRunfiles();
	ASSERT_TRUE(runfiles);

	auto test_ecsact = runfiles->Rlocation(
		"ecsact/ecsact_interpret/test/errors/cluster_invalid_conflict.ecsact"
	);

	auto errors = ecsact::eval_files({test_ecsact});
	for(auto& err : errors) {
		std::cerr << "[ERROR] " << err.error_message << " at " << err.line << ":"
							<< err.character << "\n";
	}
	ASSERT_EQ(errors.size(), 1);
	EXPECT_EQ(errors[0].eval_error, ECSACT_EVAL_ERR_INVALID_CLUSTER_SYSTEM);
}
