#include "gtest/gtest.h"
#include <vector>
#include <string>
#include <filesystem>
#include "ecsact/runtime/meta.h"
#include "ecsact/runtime/meta.hh"
#include "ecsact/interpret/eval.hh"
#include "bazel/runfiles/runfiles.hh"

namespace fs = std::filesystem;

TEST(EcsactInterpret, ExplicitClusters) {
	auto runfiles = bazel_sundry::CreateDefaultRunfiles();
	ASSERT_TRUE(runfiles);

	auto test_ecsact = runfiles->Rlocation(
		"ecsact/ecsact_interpret/test/explicit_clusters.ecsact"
	);
	ASSERT_FALSE(test_ecsact.empty());
	ASSERT_TRUE(fs::exists(test_ecsact));

	auto errors = ecsact::eval_files({test_ecsact});
	for(auto& err : errors) {
		std::cerr << "[ERROR] " << test_ecsact << ":" << err.line << ":" << err.character << " " << err.error_message << "\n";
	}
	ASSERT_TRUE(errors.empty());

	auto package_id = ecsact_meta_main_package();
	ASSERT_NE(package_id, (ecsact_package_id)-1);

	auto batch_count = ecsact_meta_count_execution_batches(package_id);
	ASSERT_EQ(batch_count, 3);

	// Batch 0: SysA
	{
		int32_t systems_count = 0;
		std::vector<ecsact_system_like_id> systems(10);
		ecsact_meta_get_execution_batch(package_id, 0, 10, systems.data(), &systems_count);
		ASSERT_EQ(systems_count, 1);
		EXPECT_STREQ(ecsact_meta_system_name(static_cast<ecsact_system_id>(systems[0])), "SysA");
	}

	// Batch 1: SysB
	{
		int32_t systems_count = 0;
		std::vector<ecsact_system_like_id> systems(10);
		ecsact_meta_get_execution_batch(package_id, 1, 10, systems.data(), &systems_count);
		ASSERT_EQ(systems_count, 1);
		EXPECT_STREQ(ecsact_meta_system_name(static_cast<ecsact_system_id>(systems[0])), "SysB");
	}

	// Batch 2: ParentSys
	{
		int32_t systems_count = 0;
		std::vector<ecsact_system_like_id> systems(10);
		ecsact_meta_get_execution_batch(package_id, 2, 10, systems.data(), &systems_count);
		ASSERT_EQ(systems_count, 1);
		auto parent_sys_id = static_cast<ecsact_system_id>(systems[0]);
		EXPECT_STREQ(ecsact_meta_system_name(parent_sys_id), "ParentSys");

		auto sys_batch_count = ecsact_meta_count_system_execution_batches(
			static_cast<ecsact_system_like_id>(parent_sys_id)
		);
		ASSERT_EQ(sys_batch_count, 2);

		// ParentSys Batch 0: ChildA
		{
			int32_t           sys_systems_count = 0;
			ecsact_system_like_id sys_systems[10];
			ecsact_meta_get_system_execution_batch(
				static_cast<ecsact_system_like_id>(parent_sys_id),
				0,
				10,
				sys_systems,
				&sys_systems_count
			);
			ASSERT_EQ(sys_systems_count, 1);
			EXPECT_STREQ(ecsact_meta_system_name(static_cast<ecsact_system_id>(sys_systems[0])), "ChildA");
		}

		// ParentSys Batch 1: ChildB
		{
			int32_t           sys_systems_count = 0;
			ecsact_system_like_id sys_systems[10];
			ecsact_meta_get_system_execution_batch(
				static_cast<ecsact_system_like_id>(parent_sys_id),
				1,
				10,
				sys_systems,
				&sys_systems_count
			);
			ASSERT_EQ(sys_systems_count, 1);
			EXPECT_STREQ(ecsact_meta_system_name(static_cast<ecsact_system_id>(sys_systems[0])), "ChildB");
		}
	}
}
