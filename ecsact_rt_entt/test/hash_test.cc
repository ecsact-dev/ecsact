#include "gtest/gtest.h"

#include "ecsact/runtime/core.hh"
#include "runtime_test.ecsact.hh"

TEST(Core, HashRegistry) {
	std::srand(std::time(nullptr));

	auto reg = ecsact::core::registry{"Hash Test"};

	auto hash_empty = reg.hash();

	for(auto i = 0; 10000 > i; ++i) {
		auto entity = reg.create_entity();
		reg.add_component(entity, runtime_test::ComponentA{std::rand()});
		reg.add_component(entity, runtime_test::ComponentB{std::rand()});
		reg.add_component<runtime_test::TriggerTag>(entity);
	}

	auto hash = reg.hash();

	ASSERT_NE(hash_empty, hash);

	auto new_entity = reg.create_entity();
	auto hash_after_new_entity = reg.hash();
	ASSERT_NE(hash, hash_after_new_entity);
	ASSERT_NE(hash_empty, hash_after_new_entity);

	reg.add_component(new_entity, runtime_test::ComponentA{std::rand()});
	auto hash_after_add_component = reg.hash();
	ASSERT_NE(hash, hash_after_add_component);
	ASSERT_NE(hash_after_new_entity, hash_after_add_component);
	ASSERT_NE(hash_empty, hash_after_add_component);

	ecsact_destroy_entity(reg.id(), new_entity);
	auto hash_after_destroy_entity = reg.hash();

	ASSERT_NE(hash_after_new_entity, hash_after_add_component);
	ASSERT_NE(hash_after_new_entity, hash_after_destroy_entity);
	ASSERT_NE(hash_empty, hash_after_destroy_entity);
	ASSERT_EQ(hash, hash_after_destroy_entity); // reset to orig

	reg.clear();
	auto hash_after_clear = reg.hash();
	ASSERT_NE(hash, hash_after_clear);
	ASSERT_EQ(hash_empty, hash_after_clear);
	ASSERT_NE(hash_after_new_entity, hash_after_clear);
}
