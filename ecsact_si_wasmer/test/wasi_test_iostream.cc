#include "wasi_test.ecsact.hh"
#include "wasi_test.ecsact.systems.h"

#include <iostream>

void wasi_test__WasiTestSystem(ecsact_system_execution_context*) {
	std::cout << "Hello from std::cout (1) " << " other thing\n";
	std::cout << "Hello from std::cout (2)\n";
}
