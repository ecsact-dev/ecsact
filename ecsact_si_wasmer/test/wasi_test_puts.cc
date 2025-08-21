#include "wasi_test.ecsact.hh"
#include "wasi_test.ecsact.systems.h"

#include <cstdio>

void wasi_test__WasiTestSystem(ecsact_system_execution_context*) {
	std::puts("puts test (1)");
	std::puts("puts test (2)");
}
