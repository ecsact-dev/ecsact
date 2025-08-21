#include "wasi_test.ecsact.hh"
#include "wasi_test.ecsact.systems.h"

#include <cstdio>
#include <vector>
#include <utility>
#include <iostream>
#include <fstream>
#include <string>

static const auto example_text = []() -> std::string {
	std::cout << "Initializing static string\n";
	auto content = std::string{};

	auto f = std::fopen("example.txt", "rb");

	if(f) {
		auto buf = std::vector<std::byte>(256, std::byte{});
		buf.resize(std::fread(buf.data(), 1, buf.size(), f));
		std::fclose(f);
		std::cout << "Static string is: " << content << "\n";
	} else {
		std::cerr << "Failed to open example.txt\n";
		std::exit(1);
	}

	return content;
}();

void wasi_test__WasiTestSystem(ecsact_system_execution_context*) {
	std::cout << "Content from file: " << example_text << "\n";
}
