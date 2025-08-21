#include "example.ecsact.hh"
#include "example.ecsact.systems.hh"

#include <iostream>
#include <cstdio>
#include <string>

void example__ExampleSystem(ecsact_system_execution_context* c_ctx) {
	example::ExampleSystem::context ctx{ecsact::execution_context{c_ctx}};
	example::ExampleSystem::impl(ctx);
}

void example::ExampleSystem::impl(context& ctx) {
	auto comp = ctx.get<example::ExampleComponent>();
	comp.num += 1;
	ctx.update(comp);

	std::cout << "Example Log from system impl comp.num=" << comp.num << "\n";
}

void example__Generator(ecsact_system_execution_context* c_ctx) {
	example::Generator::context ctx{ecsact::execution_context{c_ctx}};
	example::Generator::impl(ctx);
}

void example::Generator::impl(context& ctx) {
	ctx._ctx.generate(example::WillAdd{42});
}

void example__AddsSystem(ecsact_system_execution_context* c_ctx) {
	example::AddsSystem::context ctx{ecsact::execution_context{c_ctx}};
	example::AddsSystem::impl(ctx);
}

void example::AddsSystem::impl(context& ctx) {
	ctx.add(example::ExampleComponent{.num = 400});
}

void example__CheckShouldRemove(ecsact_system_execution_context* c_ctx) {
	example::CheckShouldRemove::context ctx{ecsact::execution_context{c_ctx}};
	example::CheckShouldRemove::impl(ctx);
}

void example::CheckShouldRemove::impl(context& ctx) {
	auto comp = ctx.get<example::ExampleComponent>();
	if(comp.num > 405) {
		ctx.add(example::WillRemove{});
	}
}

void example__RemovesSystem(ecsact_system_execution_context* c_ctx) {
	example::RemovesSystem::context ctx{ecsact::execution_context{c_ctx}};
	example::RemovesSystem::impl(ctx);
}

void example::RemovesSystem::impl(context& ctx) {
	ctx.remove<example::ExampleComponent>();
}

void example__ExampleParallelSystem(ecsact_system_execution_context* c_ctx) {
	example::ExampleParallelSystem::context ctx{ecsact::execution_context{c_ctx}};
	example::ExampleParallelSystem::impl(ctx);
}

void example::ExampleParallelSystem::impl(context& ctx) {
	auto comp = ctx.get<example::ExampleParallelComponent>();
	comp.num_para += 1;
	ctx.update(comp);
}

void example__ExampleParallelSystemParent(ecsact_system_execution_context*) {
}

void example__ExampleParallelSystemParent__ExampleParallelSystemNested(
	ecsact_system_execution_context* c_ctx
) {
	example::ExampleParallelSystemParent::ExampleParallelSystemNested::context
		ctx{ecsact::execution_context{c_ctx}};
	example::ExampleParallelSystemParent::ExampleParallelSystemNested::impl(ctx);
}

void example::ExampleParallelSystemParent::ExampleParallelSystemNested::impl(
	context& ctx
) {
	auto comp = ctx.get<example::ExampleParallelComponent>();
	comp.num_para += 1;
	ctx.update(comp);
}
