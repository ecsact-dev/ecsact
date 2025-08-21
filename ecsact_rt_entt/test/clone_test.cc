#include "gtest/gtest.h"

#include "ecsact/runtime/core.hh"
#include "runtime_test.ecsact.hh"

TEST(Core, CloneRegistry) {
	std::srand(std::time(nullptr));

	auto reg = ecsact::core::registry{"Clone Test"};

	for(auto i = 0; 10000 > i; ++i) {
		auto entity = reg.create_entity();
		reg.add_component(entity, runtime_test::ComponentA{std::rand()});
		reg.add_component(entity, runtime_test::ComponentB{std::rand()});
		reg.add_component<runtime_test::TriggerTag>(entity);
	}

	auto cloned_reg = reg.clone("Cloned Registry");

	auto original_entities = reg.get_entities();
	ASSERT_EQ(original_entities.size(), 10000); // sanity check

	auto cloned_entities = reg.get_entities();
	ASSERT_EQ(original_entities.size(), cloned_entities.size());

	for(auto entity : original_entities) {
		ASSERT_TRUE(ecsact_entity_exists(cloned_reg.id(), entity));
		ASSERT_TRUE(cloned_reg.has_component<runtime_test::ComponentA>(entity));
		ASSERT_TRUE(cloned_reg.has_component<runtime_test::ComponentB>(entity));
		ASSERT_TRUE(cloned_reg.has_component<runtime_test::TriggerTag>(entity));

		auto original_a = reg.get_component<runtime_test::ComponentA>(entity);
		auto cloned_a = cloned_reg.get_component<runtime_test::ComponentA>(entity);
		ASSERT_EQ(original_a.a, cloned_a.a);

		auto original_b = reg.get_component<runtime_test::ComponentB>(entity);
		auto cloned_b = cloned_reg.get_component<runtime_test::ComponentB>(entity);
		ASSERT_EQ(original_b.b, cloned_b.b);
	}
}
