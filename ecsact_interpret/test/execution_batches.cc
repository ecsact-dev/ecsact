#include "gtest/gtest.h"
#include <vector>
#include <string>
#include <filesystem>
#include "ecsact/runtime/meta.h"
#include "ecsact/runtime/meta.hh"
#include "ecsact/interpret/eval.hh"
#include "bazel/runfiles/runfiles.hh"

namespace fs = std::filesystem;

TEST(EcsactInterpret, ExecutionBatches) {
	auto runfiles = bazel_sundry::CreateDefaultRunfiles();
	ASSERT_TRUE(runfiles);

	auto test_ecsact = runfiles->Rlocation(
		"ecsact/ecsact_interpret/test/execution_batches.ecsact"
	);
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
	ASSERT_EQ(batch_count, 8);

	// Batch 0: SysA, SysB, MultiSys1
	{
		int32_t                            systems_count = 0;
		std::vector<ecsact_system_like_id> systems(10);
		ecsact_meta_get_execution_batch(
			package_id,
			0,
			10,
			systems.data(),
			&systems_count
		);
		ASSERT_EQ(systems_count, 3);
		EXPECT_STREQ(
			ecsact_meta_system_name(static_cast<ecsact_system_id>(systems[0])),
			"SysA"
		);
		EXPECT_STREQ(
			ecsact_meta_system_name(static_cast<ecsact_system_id>(systems[1])),
			"SysB"
		);
		EXPECT_STREQ(
			ecsact_meta_system_name(static_cast<ecsact_system_id>(systems[2])),
			"MultiSys1"
		);
	}

	// Batch 1: SysC, MultiSys2, SysD
	{
		int32_t                            systems_count = 0;
		std::vector<ecsact_system_like_id> systems(10);
		ecsact_meta_get_execution_batch(
			package_id,
			1,
			10,
			systems.data(),
			&systems_count
		);
		ASSERT_EQ(systems_count, 3);
		EXPECT_STREQ(
			ecsact_meta_system_name(static_cast<ecsact_system_id>(systems[0])),
			"SysC"
		);
		EXPECT_STREQ(
			ecsact_meta_system_name(static_cast<ecsact_system_id>(systems[1])),
			"MultiSys2"
		);
		EXPECT_STREQ(
			ecsact_meta_system_name(static_cast<ecsact_system_id>(systems[2])),
			"SysD"
		);
	}

	// Batch 2: ParentSys
	{
		int32_t                            systems_count = 0;
		std::vector<ecsact_system_like_id> systems(10);
		ecsact_meta_get_execution_batch(
			package_id,
			2,
			10,
			systems.data(),
			&systems_count
		);
		ASSERT_EQ(systems_count, 1);
		EXPECT_STREQ(
			ecsact_meta_system_name(static_cast<ecsact_system_id>(systems[0])),
			"ParentSys"
		);
	}

	// Batch 3: OtherSys, MultiSys3, IncludeSys, ExcludeSys
	{
		int32_t                            systems_count = 0;
		std::vector<ecsact_system_like_id> systems(10);
		ecsact_meta_get_execution_batch(
			package_id,
			3,
			10,
			systems.data(),
			&systems_count
		);
		ASSERT_EQ(systems_count, 4);
		EXPECT_STREQ(
			ecsact_meta_system_name(static_cast<ecsact_system_id>(systems[0])),
			"OtherSys"
		);
		EXPECT_STREQ(
			ecsact_meta_system_name(static_cast<ecsact_system_id>(systems[1])),
			"MultiSys3"
		);
		EXPECT_STREQ(
			ecsact_meta_system_name(static_cast<ecsact_system_id>(systems[2])),
			"IncludeSys"
		);
		EXPECT_STREQ(
			ecsact_meta_system_name(static_cast<ecsact_system_id>(systems[3])),
			"ExcludeSys"
		);
	}

	// Batch 4: GeneratesSys
	{
		int32_t                            systems_count = 0;
		std::vector<ecsact_system_like_id> systems(10);
		ecsact_meta_get_execution_batch(
			package_id,
			4,
			10,
			systems.data(),
			&systems_count
		);
		ASSERT_EQ(systems_count, 1);
		EXPECT_STREQ(
			ecsact_meta_system_name(static_cast<ecsact_system_id>(systems[0])),
			"GeneratesSys"
		);
	}

	// Batch 5: ConflictGenerates, MultiSys4, FinalSys
	{
		int32_t                            systems_count = 0;
		std::vector<ecsact_system_like_id> systems(10);
		ecsact_meta_get_execution_batch(
			package_id,
			5,
			10,
			systems.data(),
			&systems_count
		);
		ASSERT_EQ(systems_count, 3);
		EXPECT_STREQ(
			ecsact_meta_system_name(static_cast<ecsact_system_id>(systems[0])),
			"ConflictGenerates"
		);
		EXPECT_STREQ(
			ecsact_meta_system_name(static_cast<ecsact_system_id>(systems[1])),
			"MultiSys4"
		);
		EXPECT_STREQ(
			ecsact_meta_system_name(static_cast<ecsact_system_id>(systems[2])),
			"LastSys"
		);
	}

	// Batch 6: ParentGenerates
	{
		int32_t                            systems_count = 0;
		std::vector<ecsact_system_like_id> systems(10);
		ecsact_meta_get_execution_batch(
			package_id,
			6,
			10,
			systems.data(),
			&systems_count
		);
		ASSERT_EQ(systems_count, 1);
		EXPECT_STREQ(
			ecsact_meta_system_name(static_cast<ecsact_system_id>(systems[0])),
			"ParentGenerates"
		);
	}

	// Batch 7: AfterParent
	{
		int32_t                            systems_count = 0;
		std::vector<ecsact_system_like_id> systems(10);
		ecsact_meta_get_execution_batch(
			package_id,
			7,
			10,
			systems.data(),
			&systems_count
		);
		ASSERT_EQ(systems_count, 1);
		EXPECT_STREQ(
			ecsact_meta_system_name(static_cast<ecsact_system_id>(systems[0])),
			"AfterParent"
		);
	}
}
