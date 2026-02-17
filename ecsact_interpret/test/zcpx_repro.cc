#include "gtest/gtest.h"
#include <vector>
#include <string>
#include <filesystem>
#include "ecsact/runtime/meta.h"
#include "ecsact/runtime/meta.hh"
#include "ecsact/interpret/eval.hh"
#include "bazel/runfiles/runfiles.hh"

namespace fs = std::filesystem;

TEST(EcsactInterpret, ZcpxRepro) {
	auto runfiles = bazel_sundry::CreateDefaultRunfiles();
	ASSERT_TRUE(runfiles);

	auto test_ecsact =
		runfiles->Rlocation("ecsact/ecsact_interpret/test/zcpx_repro.ecsact");
	ASSERT_FALSE(test_ecsact.empty());
	ASSERT_TRUE(fs::exists(test_ecsact));

	auto errors = ecsact::eval_files({test_ecsact});
	for(auto& err : errors) {
		std::cerr << "[ERROR] " << test_ecsact << ":" << err.line << ":"
							<< err.character << " " << err.error_message << "\n";
	}
	ASSERT_TRUE(errors.empty());

	auto package_id = ecsact_meta_main_package();
	ASSERT_NE(package_id, (ecsact_package_id)-1);

	auto batch_count = ecsact_meta_count_execution_batches(package_id);
	ASSERT_EQ(batch_count, 1);

	int32_t systems_count = 0;
	ecsact_meta_get_execution_batch(package_id, 0, 0, nullptr, &systems_count);
	ASSERT_EQ(systems_count, 3);
}
