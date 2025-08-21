#include <string_view>
#include <span>
#include <cstdint>
#include <ranges>
#include <cstdio>
#include <iostream>
#include <filesystem>
#include <vector>
#include <fstream>
#include <format>
#include <wasm.h>
#include "tools/cpp/runfiles/runfiles.h"
#include "ecsact/si/wasmer/detail/minst/minst.hh"

namespace fs = std::filesystem;
using bazel::tools::cpp::runfiles::Runfiles;
using ecsact::wasm::detail::minst;
using ecsact::wasm::detail::minst_error;
using ecsact::wasm::detail::minst_export;
using ecsact::wasm::detail::minst_import;
using ecsact::wasm::detail::minst_import_resolve_func;
using ecsact::wasm::detail::minst_import_resolve_t;

auto read_file(fs::path p) -> std::optional<std::vector<std::byte>> {
	auto file_content = std::vector<std::byte>{};
	auto file = std::ifstream{p, std::ios::binary | std::ios::ate};
	if(!file) {
		return std::nullopt;
	}

	auto file_size = file.tellg();
	file_content.resize(file_size);

	file.seekg(0, std::ios::beg);
	file.read(reinterpret_cast<char*>(file_content.data()), file_size);

	if(!file) {
		return std::nullopt;
	}

	return file_content;
}

auto minst_test_import_fn_called = false;

auto test_guest_import_resolver( //
	const minst_import imp
) -> minst_import_resolve_t {
	if(imp.module() == "mod" && imp.name() == "minst_test_import_fn") {
		return minst_import_resolve_func{
			wasm_functype_new_0_0(),
			[](const wasm_val_vec_t* args, wasm_val_vec_t* results) -> wasm_trap_t* {
				std::cout << "minst_test_import_fn called!\n";
				minst_test_import_fn_called = true;
				return nullptr;
			},
		};
	}

	return std::nullopt;
}

auto main(int argc, char* argv[]) -> int {
	auto runfiles = Runfiles::Create(argv[0]);
	auto engine = wasm_engine_new();

	auto args = std::span(argv, argc) | std::views::drop(1) |
		std::views::transform([](auto arg) { return std::string_view(arg); });

	for(auto arg : args) {
		fs::path file_path = //
			runfiles ? runfiles->Rlocation(arg.data()) : std::string{arg};

		auto file_content = read_file(file_path);

		if(!file_content) {
			std::cerr << "Failed to read " << file_path << std::endl;
			return 1;
		}

		auto result = minst::create(
			engine,
			std::span{file_content->data(), file_content->size()},
			test_guest_import_resolver
		);

		if(std::holds_alternative<minst>(result)) {
			auto& inst = std::get<minst>(result);
			if(auto trap = inst.initialize()) {
				std::cerr //
					<< "[INITIALIZE TRAP]: " << trap->message() << std::endl;
				return 1;
			}

			auto minst_test_export_fn = std::optional<minst_export>{};

			for(auto exp : inst.exports()) {
				if(exp.name() == "minst_test_export_fn") {
					minst_test_export_fn = exp;
				}
			}

			if(minst_test_export_fn) {
				if(auto trap = minst_test_export_fn->func_call()) {
					std::cerr //
						<< "[FUNC CALL TRAP]: " << trap->message() << "\n";
					return 1;
				}

				if(!minst_test_import_fn_called) {
					std::cerr //
						<< "[TEST FAILED]: minst_test_import_fn() was not called"
						<< std::endl;
					return 1;
				}
			} else {
				std::cerr //
					<< "[TEST FAILED]: minst_test_export_fn not found" << std::endl;
				return 1;
			}

		} else {
			std::cerr << std::format( //
				"[ERROR]: {}\n",
				std::get<minst_error>(result).message
			);
			return 1;
		}
	}

	std::cout << "Test complete!\n";
	return 0;
}
