#include "gtest/gtest.h"

#include "ecsact/runtime/core.hh"
#include "ecsact/entt/registry_util.hh"
#include "ecsact/entt/detail/internal_markers.hh"
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

TEST(Core, CloneRegistryWithMarkers) {
	auto reg = ecsact::core::registry{"Clone Test With Markers"};
	auto entity = reg.create_entity();

	auto& entt_reg = ecsact::entt::get_registry(reg.id());

	entt_reg.emplace<ecsact::entt::detail::created_entity>(
		static_cast<entt::entity>(entity),
		ecsact_placeholder_entity_id{42}
	);
	entt_reg.emplace<ecsact::entt::detail::destroyed_entity>(
		static_cast<entt::entity>(entity)
	);
	entt_reg.emplace<ecsact::entt::detail::pending_add<runtime_test::ComponentA>>(
		static_cast<entt::entity>(entity),
		runtime_test::ComponentA{100}
	);
	entt_reg.emplace<ecsact::entt::detail::pending_add<runtime_test::TriggerTag>>(
		static_cast<entt::entity>(entity)
	);
	entt_reg.emplace<ecsact::entt::detail::pending_remove<runtime_test::ComponentA>>(
		static_cast<entt::entity>(entity)
	);

	auto  cloned_reg = reg.clone("Cloned Registry With Markers");
	auto& cloned_entt_reg = ecsact::entt::get_registry(cloned_reg.id());

	ASSERT_TRUE(cloned_entt_reg.all_of<ecsact::entt::detail::created_entity>(
		static_cast<entt::entity>(entity)
	));
	ASSERT_EQ(
		cloned_entt_reg.get<ecsact::entt::detail::created_entity>(
										 static_cast<entt::entity>(entity)
		)
			.placeholder_entity_id,
		ecsact_placeholder_entity_id{42}
	);

	ASSERT_TRUE(cloned_entt_reg.all_of<ecsact::entt::detail::destroyed_entity>(
		static_cast<entt::entity>(entity)
	));

	ASSERT_TRUE((cloned_entt_reg.all_of<ecsact::entt::detail::pending_add<runtime_test::ComponentA>>(
		static_cast<entt::entity>(entity)
	)));
	ASSERT_EQ(
		cloned_entt_reg.get<ecsact::entt::detail::pending_add<runtime_test::ComponentA>>(
										 static_cast<entt::entity>(entity)
		)
			.value.a,
		100
	);

	ASSERT_TRUE((cloned_entt_reg.all_of<ecsact::entt::detail::pending_add<runtime_test::TriggerTag>>(
		static_cast<entt::entity>(entity)
	)));

	ASSERT_TRUE((cloned_entt_reg.all_of<ecsact::entt::detail::pending_remove<runtime_test::ComponentA>>(
		static_cast<entt::entity>(entity)
	)));
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
