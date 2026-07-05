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

#define SETUP_NOTIFY_SYSTEM(ComponentName, Event)                       \
	auto runtime_test::ComponentName::Event::impl(context& ctx) -> void { \
	}

SETUP_NOTIFY_SYSTEM(ComponentA, OnInit)
SETUP_NOTIFY_SYSTEM(ComponentA, OnChange)
SETUP_NOTIFY_SYSTEM(ComponentA, OnRemove)
SETUP_NOTIFY_SYSTEM(ComponentB, OnInit)
SETUP_NOTIFY_SYSTEM(ComponentB, OnChange)
SETUP_NOTIFY_SYSTEM(ComponentB, OnRemove)
SETUP_NOTIFY_SYSTEM(Health, OnInit)
SETUP_NOTIFY_SYSTEM(Health, OnChange)
SETUP_NOTIFY_SYSTEM(Health, OnRemove)
SETUP_NOTIFY_SYSTEM(EmptyComponent, OnInit)
SETUP_NOTIFY_SYSTEM(EmptyComponent, OnRemove)
SETUP_NOTIFY_SYSTEM(AlwaysRemoveMe, OnInit)
SETUP_NOTIFY_SYSTEM(AlwaysRemoveMe, OnRemove)
SETUP_NOTIFY_SYSTEM(WillRemoveTrivial, OnInit)
SETUP_NOTIFY_SYSTEM(WillRemoveTrivial, OnRemove)
SETUP_NOTIFY_SYSTEM(TrivialRemoveComponent, OnInit)
SETUP_NOTIFY_SYSTEM(TrivialRemoveComponent, OnChange)
SETUP_NOTIFY_SYSTEM(TrivialRemoveComponent, OnRemove)
SETUP_NOTIFY_SYSTEM(EntityTesting, OnInit)
SETUP_NOTIFY_SYSTEM(EntityTesting, OnChange)
SETUP_NOTIFY_SYSTEM(EntityTesting, OnRemove)
SETUP_NOTIFY_SYSTEM(AddA, OnInit)
SETUP_NOTIFY_SYSTEM(AddA, OnRemove)
SETUP_NOTIFY_SYSTEM(AddB, OnInit)
SETUP_NOTIFY_SYSTEM(AddB, OnRemove)
SETUP_NOTIFY_SYSTEM(RemoveA, OnInit)
SETUP_NOTIFY_SYSTEM(RemoveA, OnRemove)
SETUP_NOTIFY_SYSTEM(RemoveB, OnInit)
SETUP_NOTIFY_SYSTEM(RemoveB, OnRemove)
SETUP_NOTIFY_SYSTEM(ParentComponent, OnInit)
SETUP_NOTIFY_SYSTEM(ParentComponent, OnChange)
SETUP_NOTIFY_SYSTEM(ParentComponent, OnRemove)
SETUP_NOTIFY_SYSTEM(NestedComponent, OnInit)
SETUP_NOTIFY_SYSTEM(NestedComponent, OnChange)
SETUP_NOTIFY_SYSTEM(NestedComponent, OnRemove)
SETUP_NOTIFY_SYSTEM(TestLazySystemComponentA, OnInit)
SETUP_NOTIFY_SYSTEM(TestLazySystemComponentA, OnChange)
SETUP_NOTIFY_SYSTEM(TestLazySystemComponentA, OnRemove)
SETUP_NOTIFY_SYSTEM(TestLazySystemComponentB, OnInit)
SETUP_NOTIFY_SYSTEM(TestLazySystemComponentB, OnChange)
SETUP_NOTIFY_SYSTEM(TestLazySystemComponentB, OnRemove)
SETUP_NOTIFY_SYSTEM(TestLazySystemTag, OnInit)
SETUP_NOTIFY_SYSTEM(TestLazySystemTag, OnRemove)
SETUP_NOTIFY_SYSTEM(TestLazySystemComponentC, OnInit)
SETUP_NOTIFY_SYSTEM(TestLazySystemComponentC, OnChange)
SETUP_NOTIFY_SYSTEM(TestLazySystemComponentC, OnRemove)
SETUP_NOTIFY_SYSTEM(NotifyComponentA, OnInit)
SETUP_NOTIFY_SYSTEM(NotifyComponentA, OnChange)
SETUP_NOTIFY_SYSTEM(NotifyComponentA, OnRemove)
SETUP_NOTIFY_SYSTEM(NotifyComponentB, OnInit)
SETUP_NOTIFY_SYSTEM(NotifyComponentB, OnChange)
SETUP_NOTIFY_SYSTEM(NotifyComponentB, OnRemove)
SETUP_NOTIFY_SYSTEM(NotifyComponentC, OnInit)
SETUP_NOTIFY_SYSTEM(NotifyComponentC, OnChange)
SETUP_NOTIFY_SYSTEM(NotifyComponentC, OnRemove)
SETUP_NOTIFY_SYSTEM(TriggerTag, OnInit)
SETUP_NOTIFY_SYSTEM(TriggerTag, OnRemove)
SETUP_NOTIFY_SYSTEM(UpdateNotifyAdd, OnInit)
SETUP_NOTIFY_SYSTEM(UpdateNotifyAdd, OnRemove)
SETUP_NOTIFY_SYSTEM(Counter, OnInit)
SETUP_NOTIFY_SYSTEM(Counter, OnChange)
SETUP_NOTIFY_SYSTEM(Counter, OnRemove)
SETUP_NOTIFY_SYSTEM(StreamTest, OnInit)
SETUP_NOTIFY_SYSTEM(StreamTest, OnChange)
SETUP_NOTIFY_SYSTEM(StreamTest, OnRemove)
SETUP_NOTIFY_SYSTEM(StreamTestToggle, OnInit)
SETUP_NOTIFY_SYSTEM(StreamTestToggle, OnChange)
SETUP_NOTIFY_SYSTEM(StreamTestToggle, OnRemove)
SETUP_NOTIFY_SYSTEM(StreamTestCounter, OnInit)
SETUP_NOTIFY_SYSTEM(StreamTestCounter, OnChange)
SETUP_NOTIFY_SYSTEM(StreamTestCounter, OnRemove)
