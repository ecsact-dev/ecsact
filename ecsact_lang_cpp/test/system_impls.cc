#include "example.ecsact.systems.hh"

void example::ExampleIndexedAction::impl(context& ctx) {
}

void example::ExampleSystemFromImports::impl(context& ctx) {
	auto a = ctx.get<pkg::a::ExampleA>();
	auto b = ctx.get<pkg::b::ExampleB>();

	a.a += 1;
	b.b += 1;

	ctx.update(a);
	ctx.update(b);
}

void example::ExampleLazy::impl(context&) {
}

void example::LazyLeap::impl(context&) {
}

void example::ExplicitlyNoLazy::impl(context&) {
}

void example::ExplicitlyNoLazyZero::impl(context&) {
}

void example::ParallelExample::impl(context&) {
}

// mock association id for sake of test
const ecsact_system_assoc_id example__AssocSystemExample__0 = {};

void example::AssocSystemExample::impl(context& ctx) {
	// sanity check
	auto b = ctx.get<pkg::b::ExampleB>();
	b.b += 1;
	ctx.update(b);

	// we can read/write assoc fields since its readable
	auto assoc_example = ctx.get<AssocFieldsExample>();
	assoc_example.f1 = 10; // NOTE: Generally you would not do this
	ctx.update(assoc_example);

	// we can access the other context
	auto other_ctx = ctx.other();

	// we can read/write to associated component
	auto assoc_a = other_ctx.get<pkg::a::ExampleA>();
	assoc_a.a += 1;
	other_ctx.update(assoc_a);
}

// mock association id for sake of test
const ecsact_system_assoc_id example__AssocSystemMultiExample__0 = {};
const ecsact_system_assoc_id example__AssocSystemMultiExample__1 = {};

void example::AssocSystemMultiExample::impl(context& ctx) {
	// sanity check
	auto a = ctx.get<pkg::a::ExampleA>();
	a.a += 1;
	ctx.update(a);

	// we have 2 associations so there are 2 other context available to us
	// the index is in declaration order (start at 0)
	auto other_ctx_0 = ctx.other<0>();
	auto other_ctx_1 = ctx.other<1>();

	// context 0 can access ExampleA
	auto assoc_a = other_ctx_0.get<pkg::a::ExampleA>();

	// context 1 can access ExampleB
	auto assoc_b = other_ctx_1.get<pkg::b::ExampleB>();
}
