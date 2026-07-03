#include <iostream>
#include "runtime_test.ecsact.hh"
#include "runtime_test.ecsact.systems.hh"

void runtime_test::SimpleSystem::impl(context& ctx) {
	auto comp = ctx.get<ComponentA>();
	comp.a += 2;
	ctx.update(comp);
}

void runtime_test::MakeAnother::impl(context& ctx) {
	ctx._ctx.generate(ctx.get<ComponentA>(), ctx.get<ComponentB>());
}

void runtime_test::TestAction::impl(context& ctx) {
}

void runtime_test::AlwaysRemove::impl(context& ctx) {
	// This trivial remove should not even be required:
	// SEE: https://github.com/ecsact-dev/ecsact_lang_cpp/issues/80
	std::cerr << "AlwaysRemove impl called (SHOULD NOT HAPPEN)\n";
	std::cerr.flush();
	std::abort();
}

void runtime_test::TrivialRemove::impl(context& ctx) {
	// This trivial remove should not even be required:
	// SEE: https://github.com/ecsact-dev/ecsact_lang_cpp/issues/80
	std::cerr << "TriviaLRemove impl called (SHOULD NOT HAPPEN)\n";
	std::cerr.flush();
	std::abort();
}

void runtime_test::SimpleIncrementImportedComp::impl(context& ctx) {
	auto comp = ctx.get<imported::test_pkg::ImportedComponent>();
	auto incrementer = ctx.get<imported::test_pkg::Incrementer>();
	comp.num += incrementer.increment_value;
	ctx.update(comp);
}

void imported::test_pkg::ImportedSystem::impl(context& ctx) {
	auto comp = ctx.get<SomeLocalComponent>();
	auto incrementer = ctx.get<Incrementer>();
	comp.local_num += incrementer.increment_value;
	ctx.update(comp);
}

void runtime_test::AddsAutoRemovedTag::impl(context& ctx) {
}

void runtime_test::TestLazySystem::impl(context& ctx) {
	auto comp_a = ctx.get<TestLazySystemComponentA>();
	auto comp_b = ctx.get<TestLazySystemComponentB>();

	comp_a.ai8 += 1;
	comp_a.au8 += 1;
	comp_a.ai16 += 1;
	comp_a.au16 += 1;
	comp_a.ai32 += 1;
	comp_a.au32 += 1;
	comp_a.af32 += 1.0;

	comp_b.num += 1;

	ctx.update(comp_a);
	ctx.update(comp_b);
}

void runtime_test::LazyParentSystem::impl(context& ctx) {
	auto comp = ctx.get<TestLazySystemComponentC>();
	comp.num_c += 1;
	ctx.update(comp);
}

void runtime_test::LazyParentSystem::LazyParentNestedSystem::impl( //
	context& ctx
) {
	auto parent_comp = ctx._ctx.parent().get<TestLazySystemComponentC>();
	auto comp = ctx.get<TestLazySystemComponentB>();
	comp.num *= parent_comp.num_c;
	ctx.update(comp);
}

auto runtime_test::LazyUpdateGeneratedEntity::impl(context& ctx) -> void {
	auto comp = ctx.get<ComponentA>();
	comp.a += 1;
	ctx.update(comp);
}

void runtime_test::TestTwoAdds::impl(context& ctx) {
}

void runtime_test::TestOneAdd::impl(context& ctx) {
}

void runtime_test::TestTwoRemoves::impl(context& ctx) {
}

void runtime_test::ParentSystem::impl(context& ctx) {
}

void runtime_test::ParentSystem::NestedSystem::impl(context& ctx) {
}

void runtime_test::InitNotify::impl(context& ctx) {
	auto comp = ctx.get<NotifyComponentA>();
	comp.val += 1;
	ctx.update(comp);
}

void runtime_test::InitNotifySelective::impl(context& ctx) {
	auto comp = ctx.get<NotifyComponentA>();
	comp.val += 1;
	ctx.update(comp);
}

void runtime_test::TriggerUpdateNotify::impl(context& ctx) {
	auto comp = ctx.get<NotifyComponentA>();
	comp.val += 1;
	ctx.update(comp);

	ctx.remove<TriggerTag>();
}

void runtime_test::UpdateNotify::impl(context& ctx) {
	ctx.add<UpdateNotifyAdd>();
}

void runtime_test::RemoveNotify::impl(context& ctx) {
	auto comp = ctx.get<NotifyComponentA>();
	comp.val += 1;
	ctx.update(comp);
}

void runtime_test::ChangeNotify::impl(context& ctx) {
	auto comp = ctx.get<NotifyComponentA>();
	comp.val += 1;
	ctx.update(comp);
}

void runtime_test::MixedNotify::impl(context& ctx) {
	auto comp = ctx.get<Counter>();
	comp.val += 1;
	ctx.update(comp);
}

void runtime_test::StreamTestSystem::impl(context& ctx) {
	auto comp = ctx.get<StreamTestCounter>();

	if(comp.val == 0) {
		ctx.stream_toggle<StreamTestToggle>(false);
	}

	if(comp.val == 10) {
		ctx.stream_toggle<StreamTestToggle>(true);
	}
}

void runtime_test::StreamTestSystemTwo::impl(context& ctx) {
	auto comp = ctx.get<StreamTestCounter>();

	if(comp.val == 0) {
		ctx.stream_toggle<StreamTest>(false);
	}

	if(comp.val == 10) {
		ctx.stream_toggle<StreamTestToggle>(true);
	}
}

void runtime_test::StreamTestSystemCounter::impl(context& ctx) {
	auto toggle_comp = ctx.get<StreamTestToggle>();

	toggle_comp.val += 10;
	ctx.update(toggle_comp);
}
