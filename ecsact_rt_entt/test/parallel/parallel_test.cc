#include "gtest/gtest.h"

#include <array>

#include "ecsact/runtime/core.hh"
#include "ecsact/runtime/dynamic.h"

#include "parallel_test.ecsact.hh"
#include "parallel_test.ecsact.systems.hh"

void parallel_test::ActionParallelA::impl(context& ctx) {
}

void parallel_test::ParentSystemConflictA::impl(context& ctx) {
}

void parallel_test::ParentSystemConflictA::NestedSystemConflictA::impl(
	context& ctx
) {
}

void parallel_test::ParentSystemConflictA::NestedSystemConflictB::impl(
	context& ctx
) {
}

void parallel_test::NestedSystemNoConflict::impl(context& ctx) {
}

void parallel_test::NestedSystemNoConflict::NestedSystemNoConflictA::impl(
	context& ctx
) {
}

void parallel_test::NestedSystemNoConflict::NestedSystemNoConflictB::impl(
	context& ctx
) {
}

void parallel_test::ReadParallelA::impl(context& ctx) {
}

void parallel_test::ReadParallelA::ReadParallelAChildSystem::impl(context& ctx
) {
}

void parallel_test::ReadWriteParallelA::impl(context& ctx) {
	auto comp = ctx.get<ParallelA>();

	comp.val += 1;
	ctx.update(comp);
}

void parallel_test::ReadWriteParallelB::impl(context& ctx) {
	auto comp = ctx.get<ParallelB>();

	comp.val += 1;
	ctx.update(comp);
}

void parallel_test::ReadParallelB::impl(context& ctx) {
}

void parallel_test::ReadParallelBB::impl(context& ctx) {
}

void parallel_test::ParentWithSharedComponentA::impl(context& ctx) {
}

void parallel_test::ParentWithSharedComponentA::ChildWithSharedComponentA::impl(
	context& ctx
) {
}

TEST(Parallel, RunEntitiesinParallel) {
	auto reg = ecsact::core::registry("RunEntitiesInParallel");

	auto entity_a = reg.create_entity();
	auto entity_b = reg.create_entity();

	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(parallel_test::ReadWriteParallelA::id
		),
		parallel_test__ReadWriteParallelA
	);

	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(parallel_test::ReadWriteParallelB::id
		),
		parallel_test__ReadWriteParallelB
	);

	reg.add_component(entity_a, parallel_test::ParallelA{});
	reg.add_component(entity_b, parallel_test::ParallelB{});

	reg.execute_systems(1000);

	auto comp_a = reg.get_component<parallel_test::ParallelA>(entity_a);
	auto comp_b = reg.get_component<parallel_test::ParallelB>(entity_b);

	ASSERT_EQ(comp_a.val, 1000);
	ASSERT_EQ(comp_b.val, 1000);

	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(parallel_test::ReadWriteParallelA::id
		),
		nullptr
	);

	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(parallel_test::ReadWriteParallelB::id
		),
		nullptr
	);
}
