#include "example.ecsact.meta.hh"

static_assert(
	ecsact::system_lazy_execution_iteration_rate_v<
		example::ExampleSystemFromImports> == 0
);

static_assert(
	ecsact::system_lazy_execution_iteration_rate_v<example::ExampleLazy> == 1
);

static_assert(
	ecsact::system_lazy_execution_iteration_rate_v<example::LazyLeap> == 24
);

static_assert(
	ecsact::system_lazy_execution_iteration_rate_v<example::ExplicitlyNoLazy> == 0
);

static_assert(
	ecsact::system_lazy_execution_iteration_rate_v<
		example::ExplicitlyNoLazyZero> == 0
);

static_assert(!ecsact::system_parallel_execution_v<example::ExampleLazy>);
static_assert(ecsact::system_parallel_execution_v<example::ParallelExample>);
