
#include "gtest/gtest.h"
#include "ecsact/interpret/eval.h"

#include "test_lib.hh"

TEST(NoPackageStatementFirst, NoPackageStatementFirst) {
	auto errs = ecsact_interpret_test_files({"comment_before_package.ecsact"});
	ASSERT_EQ(errs.size(), 0) //
		<< "Expected no errors. Instead got: " << errs[0].error_message << "\n";
}
