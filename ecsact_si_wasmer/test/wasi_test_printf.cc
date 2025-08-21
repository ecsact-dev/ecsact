#include "wasi_test.ecsact.hh"
#include "wasi_test.ecsact.systems.h"

#include <cstdio>

void wasi_test__WasiTestSystem(ecsact_system_execution_context*) {
	std::printf("printf test (1)\n");
	std::printf("printf test (2)\n");
}
