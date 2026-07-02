#include "gtest/gtest.h"

#include <array>
#include <set>
#include <typeindex>
#include <version>
#include <algorithm>
#include <random>
#include <thread>
#include <ranges>
#include <iostream>
#include "ecsact/runtime/core.hh"
#include "ecsact/runtime/dynamic.h"

#include "runtime_test.ecsact.hh"
#include "notify_codegen_test.ecsact.hh"
#include "notify_codegen_test.ecsact.notify.ecsact.hh"
#include "runtime_test.ecsact.systems.hh"

using runtime_test::ComponentA;

#define SET_SYSTEM_IMPL(SystemName)                                      \
	ASSERT_TRUE(ecsact_set_system_execution_impl(                          \
		ecsact_id_cast<ecsact_system_like_id>(runtime_test::SystemName::id), \
		&runtime_test__##SystemName                                          \
	))
#define CLEAR_SYSTEM_IMPL(SystemName)                                    \
	ASSERT_TRUE(ecsact_set_system_execution_impl(                          \
		ecsact_id_cast<ecsact_system_like_id>(runtime_test::SystemName::id), \
		nullptr                                                              \
	))

TEST(Core, CreateRegistry) {
	auto reg_id = ecsact_create_registry("CreateRegistry");
	EXPECT_NE(reg_id, ecsact_invalid_registry_id);
}

TEST(Core, DestroyRegistry) {
	auto reg_id = ecsact_create_registry("DestroyRegistry");
	ecsact_destroy_registry(reg_id);
}

TEST(Core, ClearRegistry) {
	auto                  reg_id = ecsact_create_registry("ClearRegistry");
	[[maybe_unused]] auto entity = ecsact_create_entity(reg_id);
	auto                  entity_count = ecsact_count_entities(reg_id);
	EXPECT_EQ(entity_count, 1);
	ecsact_clear_registry(reg_id);
	entity_count = ecsact_count_entities(reg_id);
	EXPECT_EQ(entity_count, 0);
}

TEST(Core, CreateEntity) {
	auto reg_id = ecsact_create_registry("CreateEntity");
	auto entity = ecsact_create_entity(reg_id);
	EXPECT_NE(entity, ecsact_invalid_entity_id);
}

TEST(Core, EnsureEntity) {
	auto reg_id = ecsact_create_registry("EnsureEntity");
	auto entity = static_cast<ecsact_entity_id>(4);
	EXPECT_FALSE(ecsact_entity_exists(reg_id, entity));
	ecsact_ensure_entity(reg_id, entity);
	EXPECT_TRUE(ecsact_entity_exists(reg_id, entity));
	ecsact_ensure_entity(reg_id, entity);
	EXPECT_TRUE(ecsact_entity_exists(reg_id, entity));
}

TEST(Core, DestroyEntity) {
	auto reg_id = ecsact_create_registry("DestroyEntity");
	auto entity = ecsact_create_entity(reg_id);
	ecsact_destroy_entity(reg_id, entity);
	EXPECT_FALSE(ecsact_entity_exists(reg_id, entity));
}

TEST(Core, CountEntities) {
	auto                  reg_id = ecsact_create_registry("CountEntities");
	[[maybe_unused]] auto entity = ecsact_create_entity(reg_id);
	EXPECT_EQ(ecsact_count_entities(reg_id), 1);
}

TEST(Core, GetEntities) {
	const int test_count = 20;

	auto reg_id = ecsact_create_registry("GetEntities");
	for(int i = 0; test_count > i; ++i) {
		ecsact_create_entity(reg_id);
	}

	EXPECT_EQ(ecsact_count_entities(reg_id), test_count);

	int                           entities_count = 0;
	std::vector<ecsact_entity_id> entities;
	entities.resize(test_count);
	for(auto& ent : entities) {
		ent = ecsact_invalid_entity_id;
	}

	ecsact_get_entities(reg_id, test_count, entities.data(), &entities_count);
	ASSERT_EQ(entities_count, test_count);

	for(auto& ent : entities) {
		EXPECT_NE(ent, ecsact_invalid_entity_id);
	}
}

TEST(Core, AddComponent) {
	auto reg_id = ecsact_create_registry("AddComponent");
	auto entity = ecsact_create_entity(reg_id);

	runtime_test::ComponentA comp{.a = 42};
	auto comp_id = static_cast<ecsact_component_id>(runtime_test::ComponentA::id);

	auto add_err = ecsact_add_component(reg_id, entity, comp_id, &comp);
	EXPECT_EQ(add_err, ECSACT_ADD_OK);

	EXPECT_TRUE(ecsact_has_component(reg_id, entity, comp_id, nullptr));
}

TEST(Core, HasComponent) {
	auto reg_id = ecsact_create_registry("HasComponent");
	auto entity = ecsact_create_entity(reg_id);

	runtime_test::ComponentA comp{.a = 42};
	auto comp_id = static_cast<ecsact_component_id>(runtime_test::ComponentA::id);

	EXPECT_FALSE(ecsact_has_component(reg_id, entity, comp_id, nullptr));
	ecsact_add_component(reg_id, entity, comp_id, &comp);
	EXPECT_TRUE(ecsact_has_component(reg_id, entity, comp_id, nullptr));
}

TEST(Core, GetComponent) {
	auto reg_id = ecsact_create_registry("GetComponent");
	auto entity = ecsact_create_entity(reg_id);

	runtime_test::ComponentA comp{.a = 42};
	auto comp_id = static_cast<ecsact_component_id>(runtime_test::ComponentA::id);
	ecsact_add_component(reg_id, entity, comp_id, &comp);

	auto comp_get = static_cast<const runtime_test::ComponentA*>(
		ecsact_get_component(reg_id, entity, comp_id, nullptr)
	);

	EXPECT_EQ(*comp_get, comp);
}

TEST(Core, UpdateComponent) {
	auto reg_id = ecsact_create_registry("UpdateComponent");
	auto entity = ecsact_create_entity(reg_id);

	runtime_test::ComponentA comp{.a = 42};
	runtime_test::ComponentA upped_comp{.a = 43};
	auto comp_id = static_cast<ecsact_component_id>(runtime_test::ComponentA::id);
	ecsact_add_component(reg_id, entity, comp_id, &comp);
	ecsact_update_component(reg_id, entity, comp_id, &upped_comp, nullptr);

	auto comp_get = static_cast<const runtime_test::ComponentA*>(
		ecsact_get_component(reg_id, entity, comp_id, nullptr)
	);

	EXPECT_EQ(*comp_get, upped_comp);
}

TEST(Core, RemoveComponent) {
	auto reg_id = ecsact_create_registry("RemoveComponent");
	auto entity = ecsact_create_entity(reg_id);

	runtime_test::ComponentA comp{.a = 42};
	auto comp_id = static_cast<ecsact_component_id>(runtime_test::ComponentA::id);
	ecsact_add_component(reg_id, entity, comp_id, &comp);
	EXPECT_TRUE(ecsact_has_component(reg_id, entity, comp_id, nullptr));
	ecsact_remove_component(reg_id, entity, comp_id, nullptr);
	EXPECT_FALSE(ecsact_has_component(reg_id, entity, comp_id, nullptr));
}

TEST(Core, DynamicSystemImpl) {
	auto reg = ecsact::core::registry("DynamicSystemImpl");
	auto entity = reg.create_entity();

	ComponentA comp{.a = 42};
	reg.add_component(entity, comp);

	// Sanity check
	ASSERT_TRUE(reg.has_component<ComponentA>(entity));
	ASSERT_EQ(reg.get_component<ComponentA>(entity), comp);

	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(runtime_test::SimpleSystem::id),
		&runtime_test__SimpleSystem
	);

	reg.execute_systems();

	// Sanity check
	ASSERT_TRUE(reg.has_component<ComponentA>(entity));

	auto comp_get = reg.get_component<ComponentA>(entity);

	EXPECT_NE(comp_get.a, comp.a);

	// Simulate what the system should be doing.
	comp.a += 2;
	EXPECT_EQ(comp_get.a, comp.a);
}

TEST(Core, MultiPkgUpdate) {
	using imported::test_pkg::ImportedComponent;
	using imported::test_pkg::Incrementer;
	using imported::test_pkg::SomeLocalComponent;

	ASSERT_TRUE(ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(
			runtime_test::SimpleIncrementImportedComp::id
		),
		&runtime_test__SimpleIncrementImportedComp
	));

	ASSERT_TRUE(ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(
			imported::test_pkg::ImportedSystem::id
		),
		&imported__test_pkg__ImportedSystem
	));

	auto reg = ecsact::core::registry("MultiPkgUpdate");
	auto test_entity = reg.create_entity();
	reg.add_component(test_entity, ImportedComponent{});
	reg.add_component(test_entity, SomeLocalComponent{});
	reg.add_component(test_entity, Incrementer{});

	for(int i = 0; 10 > i; ++i) {
		reg.update_component(test_entity, Incrementer{1});

		reg.execute_systems(1);
		{
			auto c = reg.get_component<ImportedComponent>(test_entity);
			EXPECT_EQ(c.num, i + 1);
		}

		{
			auto c = reg.get_component<SomeLocalComponent>(test_entity);
			EXPECT_EQ(c.local_num, i + 1);
		}

		reg.update_component(test_entity, Incrementer{0});
		reg.execute_systems(1);
		{
			auto c = reg.get_component<ImportedComponent>(test_entity);
			EXPECT_EQ(c.num, i + 1);
		}

		{
			auto c = reg.get_component<SomeLocalComponent>(test_entity);
			EXPECT_EQ(c.local_num, i + 1);
		}
	}
}

TEST(Core, NoAction) {
	static bool action_executed = false;

	ASSERT_TRUE(ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(runtime_test::TestAction::id),
		[](ecsact_system_execution_context*) { action_executed = true; }
	));

	auto reg = ecsact::core::registry("Core_NoAction");

	auto test_entity = reg.create_entity();
	reg.add_component(test_entity, runtime_test::ComponentA{});

	reg.execute_systems();

	ASSERT_FALSE(action_executed);

	auto exec_opts = ecsact::core::execution_options{};

	auto exec_err = reg.execute_systems(std::array{exec_opts});
	ASSERT_EQ(exec_err, ECSACT_EXEC_SYS_OK);
	ASSERT_FALSE(action_executed);
}

TEST(Core, LazySystem) {
	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(runtime_test::TestLazySystem::id),
		runtime_test__TestLazySystem
	);

	auto reg = ecsact::core::registry{"TestLazySystemRegistry"};
	auto test_entities = std::array<ecsact_entity_id, 10>{
		reg.create_entity(),
		reg.create_entity(),
		reg.create_entity(),
		reg.create_entity(),
		reg.create_entity(),
		reg.create_entity(),
		reg.create_entity(),
		reg.create_entity(),
		reg.create_entity(),
		reg.create_entity(),
	};

	for(auto entity : test_entities) {
		reg.add_component<runtime_test::TestLazySystemTag>(entity);
		reg.add_component(entity, runtime_test::TestLazySystemComponentA{});
		reg.add_component(entity, runtime_test::TestLazySystemComponentB{});
	}

	auto count_entities_with_value = [&](int num) -> int {
		auto count = 0;
		for(auto entity : test_entities) {
			auto comp =
				reg.get_component<runtime_test::TestLazySystemComponentB>(entity);

			if(comp.num == num) {
				count += 1;
			}
		}

		return count;
	};

	EXPECT_EQ(count_entities_with_value(0), 10);
	reg.execute_systems();

	EXPECT_EQ(count_entities_with_value(1), 1);

	reg.execute_systems();
	EXPECT_EQ(count_entities_with_value(1), 2);

	reg.execute_systems();
	EXPECT_EQ(count_entities_with_value(1), 3);

	reg.execute_systems();
	EXPECT_EQ(count_entities_with_value(1), 4);

	reg.execute_systems();
	EXPECT_EQ(count_entities_with_value(1), 5);

	reg.execute_systems();
	EXPECT_EQ(count_entities_with_value(1), 6);

	reg.execute_systems();
	EXPECT_EQ(count_entities_with_value(1), 7);

	reg.execute_systems();
	EXPECT_EQ(count_entities_with_value(1), 8);

	reg.execute_systems();
	EXPECT_EQ(count_entities_with_value(1), 9);

	reg.execute_systems();
	EXPECT_EQ(count_entities_with_value(1), 10);

	reg.execute_systems();
	EXPECT_EQ(count_entities_with_value(1), 9);
	EXPECT_EQ(count_entities_with_value(2), 1);
	EXPECT_EQ(count_entities_with_value(0), 0);
}

TEST(Core, LazyParentSystem) {
	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(runtime_test::LazyParentSystem::id),
		runtime_test__LazyParentSystem
	);
	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(
			runtime_test::LazyParentSystem::LazyParentNestedSystem::id
		),
		runtime_test__LazyParentSystem__LazyParentNestedSystem
	);

	auto reg = ecsact::core::registry{"LazyParentSystemRegistry"};

	auto test_entities = std::array{
		reg.create_entity(),
		reg.create_entity(),
		reg.create_entity(),
		reg.create_entity(),
		reg.create_entity(),
	};

	auto test_components_b = std::array{
		runtime_test::TestLazySystemComponentB{0},
		runtime_test::TestLazySystemComponentB{10},
		runtime_test::TestLazySystemComponentB{20},
		runtime_test::TestLazySystemComponentB{30},
		runtime_test::TestLazySystemComponentB{40},
	};

	auto test_components_c = std::array{
		runtime_test::TestLazySystemComponentC{3},
		runtime_test::TestLazySystemComponentC{13},
		runtime_test::TestLazySystemComponentC{23},
		runtime_test::TestLazySystemComponentC{33},
		runtime_test::TestLazySystemComponentC{43},
	};

	static_assert(test_entities.size() == test_components_b.size());
	static_assert(test_entities.size() == test_components_c.size());

	for(int i = 0; test_entities.size() > i; ++i) {
		reg.add_component(test_entities[i], test_components_b[i]);
		reg.add_component(test_entities[i], test_components_c[i]);
	}

	// sanity check
	for(int i = 0; test_entities.size() > i; ++i) {
		auto comp_b = reg.get_component<runtime_test::TestLazySystemComponentB>( //
			test_entities[i]
		);
		auto comp_c = reg.get_component<runtime_test::TestLazySystemComponentC>( //
			test_entities[i]
		);
		ASSERT_EQ(comp_b.num, test_components_b[i].num);
		ASSERT_EQ(comp_c.num_c, test_components_c[i].num_c);
	}

	auto changed_index = -1;
	auto changed_components_b = decltype(test_components_b){};
	auto changed_components_c = decltype(test_components_c){};

#define CALC_CHANGED_INDEX()                                                  \
	changed_index = -1;                                                         \
	for(int i = 0; test_entities.size() > i; ++i) {                             \
		auto comp = reg.get_component<runtime_test::TestLazySystemComponentB>(    \
			test_entities[i]                                                        \
		);                                                                        \
		changed_components_b[i] = comp;                                           \
	}                                                                           \
	for(int i = 0; test_entities.size() > i; ++i) {                             \
		auto comp = reg.get_component<runtime_test::TestLazySystemComponentC>(    \
			test_entities[i]                                                        \
		);                                                                        \
		changed_components_c[i] = comp;                                           \
	}                                                                           \
                                                                              \
	for(int i = 0; test_entities.size() > i; ++i) {                             \
		if(changed_components_c[i].num_c != test_components_c[i].num_c) {         \
			ASSERT_EQ(changed_index, -1) << "more than one test component changed"; \
			changed_index = i;                                                      \
		}                                                                         \
	}                                                                           \
	static_assert(true, "macro requires ;")

	reg.execute_systems();
	CALC_CHANGED_INDEX();
	ASSERT_NE(changed_index, -1);
	auto initial_changed_component = test_components_c[changed_index];

	for([[maybe_unused]] auto i :
			std::views::iota(0U, test_entities.size() - 1)) {
		reg.execute_systems();
	}

	auto rd = std::random_device{};
	auto g = std::mt19937(rd());

	for(auto attempt = 0; 100 > attempt; ++attempt) {
		std::shuffle(test_components_c.begin(), test_components_c.end(), g);

		changed_index = std::distance(
			test_components_c.begin(),
			std::find(
				test_components_c.begin(),
				test_components_c.end(),
				initial_changed_component
			)
		);

		ASSERT_NE(changed_index, -1);
		ASSERT_LT(changed_index, test_components_c.size());

		for(int i = 0; test_entities.size() > i; ++i) {
			reg.update_component<runtime_test::TestLazySystemComponentB>(
				test_entities[i],
				test_components_b[i]
			);
			reg.update_component<runtime_test::TestLazySystemComponentC>(
				test_entities[i],
				test_components_c[i]
			);
		}
		CALC_CHANGED_INDEX(); // sanity check
		ASSERT_EQ(changed_index, -1);

		reg.execute_systems();
		CALC_CHANGED_INDEX();
		ASSERT_NE(changed_index, -1);

		// We're expecting each iteration to change the same component
		EXPECT_EQ(
			initial_changed_component.num_c,
			test_components_c[changed_index].num_c
		) << "different component was changed with same set of data\nindex="
			<< changed_index;

		for([[maybe_unused]] auto _ :
				std::views::iota(0U, test_entities.size() - 1)) {
			reg.execute_systems();
		}
	}

#undef CALC_CHANGED_INDEX
}

TEST(Core, LazyUpdateGeneratedEntity) {
	using runtime_test::ComponentA;
	using runtime_test::ComponentB;
	using runtime_test::MakeAnother;

	// these systems mess with ComponentA
	CLEAR_SYSTEM_IMPL(AddsAutoRemovedTag);
	CLEAR_SYSTEM_IMPL(SimpleSystem);
	CLEAR_SYSTEM_IMPL(TestAction);

	auto reg = ecsact::core::registry{"Core_LazyUpdateGeneratedEntity"};

	auto test_entity = reg.create_entity();
	reg.add_component(test_entity, ComponentA{});
	reg.add_component(test_entity, ComponentB{});

	auto exec_opts = ecsact::core::execution_options{};
	auto make_another_action = MakeAnother{};
	exec_opts.push_action(&make_another_action);

	// generate 7 more entities from MakeAnother system impl
	SET_SYSTEM_IMPL(MakeAnother);
	auto err = reg.execute_systems(std::array{exec_opts, exec_opts, exec_opts});
	ASSERT_EQ(err, ECSACT_EXEC_SYS_OK);
	ASSERT_EQ(reg.count_entities(), 8);
	CLEAR_SYSTEM_IMPL(MakeAnother);

	auto count_entities_with_value = [&](int num) -> int {
		auto count = 0;
		for(auto entity : reg.get_entities()) {
			auto comp = reg.get_component<ComponentA>(entity);

			if(comp.a == num) {
				count += 1;
			}
		}

		return count;
	};

	// this system should increment ComponentA::a one by one
	SET_SYSTEM_IMPL(LazyUpdateGeneratedEntity);

	reg.execute_systems();
	EXPECT_EQ(count_entities_with_value(1), 1);

	reg.execute_systems();
	EXPECT_EQ(count_entities_with_value(1), 2);

	reg.execute_systems();
	EXPECT_EQ(count_entities_with_value(1), 3);

	reg.execute_systems();
	EXPECT_EQ(count_entities_with_value(1), 4);

	reg.execute_systems();
	EXPECT_EQ(count_entities_with_value(1), 5);
}

#ifdef ECSACT_ENTT_TEST_STATIC_SYSTEM_IMPL
TEST(Core, StaticSystemImpl) {
	auto reg_id = ecsact_create_registry("StaticSystemImpl");
	auto entity = ecsact_create_entity(reg_id);

	runtime_test::ComponentA comp{.a = 42};
	auto comp_id = static_cast<ecsact_component_id>(runtime_test::ComponentA::id);
	ecsact_add_component(reg_id, entity, comp_id, &comp);

	auto comp_get = static_cast<const runtime_test::ComponentA*>(
		ecsact_get_component(reg_id, entity, runtime_test::ComponentA::id, nullptr)
	);

	// Sanity check
	ASSERT_EQ(comp_get->a, comp.a);

	// Clear any system impls that may already be set so we can use the static
	// one
	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(runtime_test::SimpleSystem::id),
		nullptr
	);

	ecsact_execute_systems(reg_id, 1, nullptr);

	comp_get = static_cast<const runtime_test::ComponentA*>(
		ecsact_get_component(reg_id, entity, comp_id, nullptr)
	);

	EXPECT_NE(comp_get->a, comp.a);

	// Simulate what the system should be doing.
	comp.a += 2;
	EXPECT_EQ(comp_get->a, comp.a);
}
#endif // ECSACT_ENTT_TEST_STATIC_SYSTEM_IMPL

TEST(Core, NotifyOnInit) {
	using runtime_test::NotifyComponentA;
	using runtime_test::NotifyComponentB;
	using runtime_test::NotifyComponentC;

	auto reg = ecsact::core::registry("NotifyOnInit");

	auto entity = reg.create_entity();

	auto notify_comp_a = NotifyComponentA{.val = 0};
	auto notify_comp_b = NotifyComponentB{.val = 0};
	auto notify_comp_c = NotifyComponentC{.val = 0};

	auto exec_options = ecsact::core::execution_options{};

	exec_options.add_component(entity, &notify_comp_a);
	exec_options.add_component(entity, &notify_comp_b);
	exec_options.add_component(entity, &notify_comp_c);

	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(runtime_test::InitNotify::id),
		runtime_test__InitNotify
	);

	auto error = reg.execute_systems(std::array{exec_options});
	ASSERT_EQ(error, ECSACT_EXEC_SYS_OK);
	reg.execute_systems(5);
	auto updated_comp_a = reg.get_component<NotifyComponentA>(entity);

	ASSERT_EQ(updated_comp_a.val, 1);

	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(runtime_test::InitNotify::id),
		nullptr
	);
}

TEST(Core, NotifyOnInitSelective) {
	using runtime_test::NotifyComponentA;
	using runtime_test::NotifyComponentB;
	using runtime_test::NotifyComponentC;

	auto reg = ecsact::core::registry("NotifyOnInitSelective");

	auto entity = reg.create_entity();

	auto notify_comp_a = NotifyComponentA{.val = 0};
	auto notify_comp_b = NotifyComponentB{.val = 0};
	auto notify_comp_c = NotifyComponentC{.val = 0};

	auto exec_options = ecsact::core::execution_options{};

	exec_options.add_component(entity, &notify_comp_a);
	exec_options.add_component(entity, &notify_comp_b);
	exec_options.add_component(entity, &notify_comp_c);

	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(
			runtime_test::InitNotifySelective::id
		),
		runtime_test__InitNotifySelective
	);

	auto error = reg.execute_systems(std::array{exec_options});
	ASSERT_EQ(error, ECSACT_EXEC_SYS_OK);
	reg.execute_systems(5);
	auto updated_comp_a = reg.get_component<NotifyComponentA>(entity);

	ASSERT_EQ(updated_comp_a.val, 1);

	exec_options.clear();
	exec_options.remove_component(entity, notify_comp_b.id);
	error = reg.execute_systems(std::array{exec_options});
	ASSERT_EQ(error, ECSACT_EXEC_SYS_OK);

	exec_options.clear();
	exec_options.add_component(entity, &notify_comp_b);
	error = reg.execute_systems(std::array{exec_options});
	ASSERT_EQ(error, ECSACT_EXEC_SYS_OK);
	reg.execute_systems(5);

	updated_comp_a = reg.get_component<NotifyComponentA>(entity);
	ASSERT_EQ(updated_comp_a.val, 2);

	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(
			runtime_test::InitNotifySelective::id
		),
		nullptr
	);
}

TEST(Core, NotifyOnRemove) {
	using runtime_test::NotifyComponentA;
	using runtime_test::NotifyComponentB;
	using runtime_test::NotifyComponentC;

	auto reg = ecsact::core::registry("NotifyOnRemove");

	auto entity = reg.create_entity();

	auto notify_comp_a = NotifyComponentA{.val = 0};
	auto notify_comp_b = NotifyComponentB{.val = 0};
	auto notify_comp_c = NotifyComponentC{.val = 0};

	auto exec_options = ecsact::core::execution_options{};

	exec_options.add_component(entity, &notify_comp_a);
	exec_options.add_component(entity, &notify_comp_b);
	exec_options.add_component(entity, &notify_comp_c);

	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(runtime_test::RemoveNotify::id),
		runtime_test__RemoveNotify
	);

	auto error = reg.execute_systems(std::array{exec_options});
	ASSERT_EQ(error, ECSACT_EXEC_SYS_OK);
	reg.execute_systems(5);
	auto updated_comp_a = reg.get_component<NotifyComponentA>(entity);
	ASSERT_EQ(updated_comp_a.val, 0);

	exec_options.clear();
	exec_options.remove_component(entity, notify_comp_b.id);
	error = reg.execute_systems(std::array{exec_options});
	ASSERT_EQ(error, ECSACT_EXEC_SYS_OK);

	updated_comp_a = reg.get_component<NotifyComponentA>(entity);
	ASSERT_EQ(updated_comp_a.val, 1);

	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(runtime_test::RemoveNotify::id),
		nullptr
	);
}

TEST(Core, NotifyOnSystemUpdate) {
	using runtime_test::NotifyComponentA;
	using runtime_test::TriggerTag;
	using runtime_test::UpdateNotifyAdd;

	auto reg = ecsact::core::registry("NotifyOnSystemUpdate");

	auto entity = reg.create_entity();

	auto notify_comp_a = NotifyComponentA{.val = 0};
	auto trigger_tag = TriggerTag{};

	auto exec_options = ecsact::core::execution_options{};

	exec_options.add_component(entity, &notify_comp_a);
	exec_options.add_component(entity, &trigger_tag);

	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(
			runtime_test::TriggerUpdateNotify::id
		),
		runtime_test__TriggerUpdateNotify
	);

	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(runtime_test::UpdateNotify::id),
		runtime_test__UpdateNotify
	);

	auto error = reg.execute_systems(std::array{exec_options});
	ASSERT_EQ(error, ECSACT_EXEC_SYS_OK);
	notify_comp_a = reg.get_component<NotifyComponentA>(entity);
	ASSERT_EQ(notify_comp_a.val, 1);

	reg.execute_systems(5);
	notify_comp_a = reg.get_component<NotifyComponentA>(entity);
	ASSERT_EQ(notify_comp_a.val, 1);

	EXPECT_TRUE(reg.has_component<UpdateNotifyAdd>(entity));

	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(
			runtime_test::TriggerUpdateNotify::id
		),
		nullptr
	);

	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(runtime_test::UpdateNotify::id),
		nullptr
	);
}

TEST(Core, NotifyOnChange) {
	using runtime_test::NotifyComponentA;

	auto reg = ecsact::core::registry("NotifyOnSystemUpdate");

	auto entity = reg.create_entity();

	auto notify_comp_a = NotifyComponentA{.val = 0};

	auto exec_options = ecsact::core::execution_options{};

	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(runtime_test::ChangeNotify::id),
		runtime_test__ChangeNotify
	);

	exec_options.add_component(entity, &notify_comp_a);

	auto error = reg.execute_systems(std::array{exec_options});
	ASSERT_EQ(error, ECSACT_EXEC_SYS_OK);

	notify_comp_a = reg.get_component<NotifyComponentA>(entity);
	ASSERT_EQ(notify_comp_a.val, 0);

	exec_options.clear();
	exec_options.update_component(entity, &notify_comp_a);

	error = reg.execute_systems(std::array{exec_options});
	ASSERT_EQ(error, ECSACT_EXEC_SYS_OK);

	// Should not call on change
	notify_comp_a = reg.get_component<NotifyComponentA>(entity);
	ASSERT_EQ(notify_comp_a.val, 0);

	notify_comp_a.val = 5;

	exec_options.clear();
	exec_options.update_component(entity, &notify_comp_a);

	// System should update, increasing value by 1 to 6
	error = reg.execute_systems(std::array{exec_options});
	ASSERT_EQ(error, ECSACT_EXEC_SYS_OK);

	notify_comp_a = reg.get_component<NotifyComponentA>(entity);
	ASSERT_EQ(notify_comp_a.val, 6);

	exec_options.clear();
	exec_options.update_component(entity, &notify_comp_a);

	error = reg.execute_systems(std::array{exec_options});
	ASSERT_EQ(error, ECSACT_EXEC_SYS_OK);

	notify_comp_a = reg.get_component<NotifyComponentA>(entity);
	ASSERT_EQ(notify_comp_a.val, 6);

	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(runtime_test::ChangeNotify::id),
		nullptr
	);
}

TEST(Core, NotifyMixed) {
	using runtime_test::Counter;
	using runtime_test::NotifyComponentA;
	using runtime_test::NotifyComponentB;
	using runtime_test::NotifyComponentC;

	auto reg = ecsact::core::registry("NotifyMixed");

	auto entity = reg.create_entity();

	auto notify_comp_a = NotifyComponentA{.val = 0};
	auto notify_comp_b = NotifyComponentB{.val = 0};
	auto notify_comp_c = NotifyComponentC{.val = 0};
	auto counter = Counter{.val = 0};

	auto exec_options = ecsact::core::execution_options{};

	exec_options.add_component(entity, &notify_comp_a);
	exec_options.add_component(entity, &notify_comp_b);
	exec_options.add_component(entity, &notify_comp_c);
	exec_options.add_component(entity, &counter);

	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(runtime_test::MixedNotify::id),
		runtime_test__MixedNotify
	);

	auto error = reg.execute_systems(std::array{exec_options});
	ASSERT_EQ(error, ECSACT_EXEC_SYS_OK);

	counter = reg.get_component<Counter>(entity);
	ASSERT_EQ(counter.val, 1);

	reg.execute_systems(1);

	counter = reg.get_component<Counter>(entity);
	ASSERT_EQ(counter.val, 1);

	exec_options.clear();

	notify_comp_a.val = 100;

	exec_options.update_component(entity, &notify_comp_a);

	error = reg.execute_systems(std::array{exec_options});
	ASSERT_EQ(error, ECSACT_EXEC_SYS_OK);

	counter = reg.get_component<Counter>(entity);
	ASSERT_EQ(counter.val, 2);

	reg.execute_systems(1);

	counter = reg.get_component<Counter>(entity);
	ASSERT_EQ(counter.val, 2);

	exec_options.clear();
	exec_options.remove_component<NotifyComponentB>(entity);

	error = reg.execute_systems(std::array{exec_options});
	ASSERT_EQ(error, ECSACT_EXEC_SYS_OK);

	counter = reg.get_component<Counter>(entity);
	ASSERT_EQ(counter.val, 3);

	reg.execute_systems(1);
	ASSERT_EQ(error, ECSACT_EXEC_SYS_OK);

	counter = reg.get_component<Counter>(entity);
	ASSERT_EQ(counter.val, 3);
}

TEST(Core, StreamComponent) {
	using runtime_test::StreamTest;

	auto reg = ecsact::core::registry("Stream");
	auto entity = reg.create_entity();
	auto exec_options = ecsact::core::execution_options{};

	auto stream_component = StreamTest{.val = 0};

	exec_options.add_component(entity, &stream_component);

	auto error = reg.execute_systems(std::array{exec_options});
	int  prev_val = 0;

	for(int i = 0; i < 100; i++) {
		stream_component.val += 10;
		ecsact_stream(reg.id(), entity, StreamTest::id, &stream_component, nullptr);
		reg.execute_systems();

		stream_component = reg.get_component<StreamTest>(entity);
		ASSERT_EQ(stream_component.val, prev_val + 10);
		prev_val = stream_component.val;
	}
}

TEST(Core, StreamComponentMultiThreadedOneEntity) {
	using runtime_test::StreamTest;

	auto reg = ecsact::core::registry("Stream");
	auto entity = reg.create_entity();
	auto exec_options = ecsact::core::execution_options{};

	auto thread_pool = std::array<std::thread, 4>{};

	auto stream_component = StreamTest{.val = 0};

	exec_options.add_component(entity, &stream_component);

	auto error = reg.execute_systems(std::array{exec_options});
	int  prev_val = 0;

	for(auto& thread : thread_pool) {
		thread = std::thread([&, reg_id = reg.id(), entity] {
			auto stream_component = StreamTest{.val = 0};
			for(int i = 0; i < 10; ++i) {
				ecsact_stream(
					reg_id,
					entity,
					StreamTest::id,
					&stream_component,
					nullptr
				);
			}
		});
	}

	for(int i = 0; i < 5; i++) {
		reg.execute_systems();
	}

	for(auto& thread : thread_pool) {
		thread.join();
	}
}

TEST(Core, StreamComponentToggle) {
	using runtime_test::StreamTestCounter;
	using runtime_test::StreamTestToggle;

	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(runtime_test::StreamTestSystem::id),
		runtime_test__StreamTestSystem
	);

	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(
			runtime_test::StreamTestSystemCounter::id
		),
		runtime_test__StreamTestSystemCounter
	);

	auto reg = ecsact::core::registry("Stream");
	auto entity = reg.create_entity();
	auto exec_options = ecsact::core::execution_options{};

	auto stream_component = StreamTestToggle{.val = 0};
	auto stream_comp_counter = StreamTestCounter{.val = 0};

	exec_options.add_component(entity, &stream_component);
	exec_options.add_component(entity, &stream_comp_counter);

	auto error = reg.execute_systems(std::array{exec_options});
	int  prev_val = 10;

	for(int i = 0; i < 5; i++) {
		stream_component.val += 10;
		ecsact_stream(
			reg.id(),
			entity,
			StreamTestToggle::id,
			&stream_component,
			nullptr
		);

		reg.execute_systems();

		stream_component = reg.get_component<StreamTestToggle>(entity);
		stream_comp_counter = reg.get_component<StreamTestCounter>(entity);
		ASSERT_EQ(stream_component.val, prev_val + 10);
		prev_val = stream_component.val;
	}

	stream_comp_counter.val += 10;
	exec_options.clear();
	exec_options.update_component(entity, &stream_comp_counter);
	error = reg.execute_systems(std::array{exec_options});

	for(int i = 0; i < 5; i++) {
		stream_component.val += 10;
		ecsact_stream(
			reg.id(),
			entity,
			StreamTestToggle::id,
			&stream_component,
			nullptr
		);

		reg.execute_systems();

		stream_component = reg.get_component<StreamTestToggle>(entity);
		stream_comp_counter = reg.get_component<StreamTestCounter>(entity);
		ASSERT_EQ(stream_component.val, prev_val + 10);
		prev_val = stream_component.val;
	}

	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(runtime_test::StreamTestSystem::id),
		nullptr
	);

	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(
			runtime_test::StreamTestSystemCounter::id
		),
		nullptr
	);
}

static int on_init_calls = 0;
static int on_change_calls = 0;
static int on_remove_calls = 0;

static void my_on_init_impl(ecsact_system_execution_context* ctx) {
	on_init_calls++;
}

static void my_on_change_impl(ecsact_system_execution_context* ctx) {
	on_change_calls++;
}

static void my_on_remove_impl(ecsact_system_execution_context* ctx) {
	on_remove_calls++;
}

TEST(Core, NotifyCodegenPlugin) {
	using notify_codegen_test::NotifyCodegenComponent;

	auto reg = ecsact::core::registry("NotifyCodegenPlugin");

	auto entity = reg.create_entity();

	on_init_calls = 0;
	on_change_calls = 0;
	on_remove_calls = 0;

	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(NotifyCodegenComponent::OnInit::id),
		&my_on_init_impl
	);
	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(NotifyCodegenComponent::OnChange::id),
		&my_on_change_impl
	);
	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(NotifyCodegenComponent::OnRemove::id),
		&my_on_remove_impl
	);

	// 1. Add component -> OnInit
	{
		auto exec_options = ecsact::core::execution_options{};
		auto comp = NotifyCodegenComponent{.val = 0};
		exec_options.add_component(entity, &comp);
		auto error = reg.execute_systems(std::array{exec_options});
		ASSERT_EQ(error, ECSACT_EXEC_SYS_OK);
		ASSERT_EQ(on_init_calls, 1);
		ASSERT_EQ(on_change_calls, 0);
		ASSERT_EQ(on_remove_calls, 0);
	}

	// 2. Update component -> OnChange
	{
		auto exec_options = ecsact::core::execution_options{};
		auto comp = NotifyCodegenComponent{.val = 20};
		exec_options.update_component(entity, &comp);
		auto error = reg.execute_systems(std::array{exec_options});
		ASSERT_EQ(error, ECSACT_EXEC_SYS_OK);
		ASSERT_EQ(on_init_calls, 1);
		ASSERT_EQ(on_change_calls, 1);
		ASSERT_EQ(on_remove_calls, 0);
	}

	// 3. Remove component -> OnRemove
	{
		auto exec_options = ecsact::core::execution_options{};
		exec_options.remove_component<NotifyCodegenComponent>(entity);
		auto error = reg.execute_systems(std::array{exec_options});
		ASSERT_EQ(error, ECSACT_EXEC_SYS_OK);
		ASSERT_EQ(on_init_calls, 1);
		ASSERT_EQ(on_change_calls, 1);
		ASSERT_EQ(on_remove_calls, 1);
	}

	// Cleanup
	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(NotifyCodegenComponent::OnInit::id),
		nullptr
	);
	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(NotifyCodegenComponent::OnChange::id),
		nullptr
	);
	ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(NotifyCodegenComponent::OnRemove::id),
		nullptr
	);
}
