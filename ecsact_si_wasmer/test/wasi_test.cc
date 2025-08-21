#include <vector>
#include <atomic>
#include <string>
#include <filesystem>
#include <iostream>
#include <csignal>
#include "magic_enum.hpp"
#include "ecsact/runtime/core.h"
#include "ecsact/runtime/core.hh"
#include "ecsact/si/wasm.h"

#include "wasi_test.ecsact.hh"

namespace fs = std::filesystem;

void print_load_error(ecsact_si_wasm_error err, const std::string& wasm_path) {
	std::cerr //
		<< "[ERROR] loading wasm file " << wasm_path
		<< " failed: " << magic_enum::enum_name(err) << "\n";

	auto err_msg_len = ecsact_si_wasm_last_error_message_length();
	auto err_msg = std::string{};
	err_msg.resize(err_msg_len);
	ecsact_si_wasm_last_error_message(err_msg.data(), err_msg_len);
	std::cerr << err_msg << "\n";
}

std::atomic_bool wasm_trap_happened = false;

void trap_handler(ecsact_system_like_id system_id, const char* trap_message) {
	wasm_trap_happened = true;
	std::string msg = "[WASM TRAP]: ";
	msg += std::string(trap_message);
	msg += "\n";

	std::cerr << msg;
	std::cerr.flush();
}

auto load_wasm_files(const std::vector<std::string>& wasm_file_paths) {
	for(auto& wasm_path : wasm_file_paths) {
		auto system_ids = std::vector<ecsact_system_like_id>{
			ecsact_id_cast<ecsact_system_like_id>(wasi_test::WasiTestSystem::id),
		};

		auto wasm_exports = std::vector<const char*>{
			"wasi_test__WasiTestSystem",
		};

		assert(system_ids.size() == wasm_exports.size());

		auto err = ecsact_si_wasm_load_file(
			wasm_path.c_str(),
			static_cast<int>(system_ids.size()),
			system_ids.data(),
			wasm_exports.data()
		);

		if(err != ECSACT_SI_WASM_OK) {
			print_load_error(err, wasm_path);
			return std::exit(2);
		}

		std::cout << "Loaded " << wasm_path << " successfully!\n";
	}
}

auto parse_args(int argc, char* argv[]) -> std::vector<std::string> {
	auto wasm_file_paths = std::vector<std::string>{};

	for(int i = 1; argc > i; ++i) {
		std::string arg(argv[i]);

		if(arg.starts_with('-')) {
			std::cerr << "[ERROR] Unknown arg: " << arg << "\n";
			std::exit(1);
		} else {
			wasm_file_paths.push_back(arg);
		}
	}

	if(wasm_file_paths.empty()) {
		std::cerr << "[ERROR] Nothing to do.\n";
		std::exit(1);
	}

	for(auto& wasm_path : wasm_file_paths) {
		if(!fs::exists(wasm_path)) {
			std::cerr << "[ERROR] " << wasm_path << " does not exist\n";
			std::exit(1);
		}
	}

	return wasm_file_paths;
}

auto load_test_virtual_files() -> void {
	auto real_path = std::string{
		"C:/Users/zekew/projects/ecsact-dev/ecsact_si_wasm/example/"
		"wasi_test_file.txt",
	};
	auto virtual_path = std::string{"example.txt"};

	std::cout << "Mapping " << real_path << " -> " << virtual_path << "\n";
	ecsact_si_wasm_allow_file_read_access(
		real_path.c_str(),
		static_cast<int32_t>(real_path.size()),
		virtual_path.c_str(),
		static_cast<int32_t>(virtual_path.size())
	);
}

auto forward_logs_consumer(
	ecsact_si_wasm_log_level log_level,
	const char*              message,
	int32_t                  message_length,
	void*                    user_data
) -> void {
	using namespace std::string_view_literals;
	auto message_view = std::string_view(message, message_length);

	auto out_iostream = std::ref(std::cout);
	if(log_level == ECSACT_SI_WASM_LOG_LEVEL_ERROR) {
		out_iostream = std::ref(std::cerr);
	}

	out_iostream.get() //
		<< "["
		<< magic_enum::enum_name(log_level).substr(
				 "ECSACT_SI_WASM_LOG_LEVEL_"sv.size()
			 )
		<< "] " << message_view << "\n";
}

int main(int argc, char* argv[]) {
	auto wasm_file_paths = parse_args(argc, argv);

	ecsact::core::registry test_registry("Test Registry");

	auto test_entity = test_registry.create_entity();
	test_registry.add_component(test_entity, wasi_test::DummyComponent{});

	ecsact_si_wasm_set_trap_handler(&trap_handler);
	load_test_virtual_files();
	load_wasm_files(wasm_file_paths);
	ecsact_si_wasm_consume_logs(forward_logs_consumer, nullptr);

	for(int i = 0; 10 > i; ++i) {
		std::cout << "\n==== EXECUTION (" << i << ") ====\n";
		ecsact_execute_systems(test_registry.id(), 1, nullptr, nullptr);
		ecsact_si_wasm_consume_logs(forward_logs_consumer, nullptr);
	}

	std::cout << "\n (( Done ))\n";

	if(wasm_trap_happened) {
		return 1;
	}

	return 0;
}
