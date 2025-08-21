#include <vector>
#include <atomic>
#include <string>
#include <filesystem>
#include <iostream>
#include <csignal>
#include <span>
#include <array>
#include <format>
#include "magic_enum.hpp"
#include "ecsact/runtime/core.h"
#include "ecsact/runtime/core.hh"
#include "ecsact/si/wasm.hh"

#include "example.ecsact.hh"

namespace fs = std::filesystem;
using namespace std::string_view_literals;

const auto test_systems = std::array{
	ecsact::si::wasm::system_load_options{
		ecsact_id_cast<ecsact_system_like_id>(example::ExampleSystem::id),
		"example__ExampleSystem"sv,
	},
	ecsact::si::wasm::system_load_options{
		ecsact_id_cast<ecsact_system_like_id>(example::ExampleParallelSystem::id),
		"example__ExampleParallelSystem"sv,
	},
	ecsact::si::wasm::system_load_options{
		ecsact_id_cast<ecsact_system_like_id>(
			example::ExampleParallelSystemParent::id
		),
		"example__ExampleParallelSystemParent"sv,
	},
	ecsact::si::wasm::system_load_options{
		ecsact_id_cast<ecsact_system_like_id>(
			example::ExampleParallelSystemParent::ExampleParallelSystemNested::id
		),
		"example__ExampleParallelSystemParent__ExampleParallelSystemNested"sv,
	},
	ecsact::si::wasm::system_load_options{
		ecsact_id_cast<ecsact_system_like_id>(example::Generator::id),
		"example__Generator"sv,
	},
	ecsact::si::wasm::system_load_options{
		ecsact_id_cast<ecsact_system_like_id>(example::AddsSystem::id),
		"example__AddsSystem"sv,
	},
	ecsact::si::wasm::system_load_options{
		ecsact_id_cast<ecsact_system_like_id>(example::CheckShouldRemove::id),
		"example__CheckShouldRemove"sv,
	},
	ecsact::si::wasm::system_load_options{
		ecsact_id_cast<ecsact_system_like_id>(example::RemovesSystem::id),
		"example__RemovesSystem"sv,
	},
};

void print_load_error(ecsact_si_wasm_error err, const std::string& wasm_path) {
	std::cerr << std::format(
		"[ERROR] loading wasm file {} failed: {}\n{}\n",
		wasm_path,
		magic_enum::enum_name(err),
		ecsact::si::wasm::last_error_message()
	);
}

std::string system_name(ecsact_system_like_id sys_id) {
	using namespace example;

	if(sys_id == ecsact_id_cast<ecsact_system_like_id>(Generator::id)) {
		return "example.Generator";
	}
	if(sys_id == ecsact_id_cast<ecsact_system_like_id>(AddsSystem::id)) {
		return "example.AddsSystem";
	}
	if(sys_id == ecsact_id_cast<ecsact_system_like_id>(RemovesSystem::id)) {
		return "example.RemovesSystem";
	}
	if(sys_id == ecsact_id_cast<ecsact_system_like_id>(ExampleSystem::id)) {
		return "example.ExampleSystem";
	}

	return "UNKNOWN SYSTEM OR ACTION";
}

std::string component_name(ecsact_component_id comp_id) {
	if(comp_id == example::WillRemove::id) {
		return "example.WillRemove";
	}
	if(comp_id == example::WillAdd::id) {
		return "example.WillAdd";
	}
	if(comp_id == example::WillAdd::id) {
		return "example.WillAdd";
	}
	if(comp_id == example::Spawner::id) {
		return "example.Spawner";
	}
	if(comp_id == example::ExampleComponent::id) {
		return "example.ExampleComponent";
	}
	if(comp_id == example::ExampleParallelComponent::id) {
		return "example.ExampleParallelComponent";
	}

	return "UNKNOWN COMPONENT";
}

void print_event(
	ecsact_event        event,
	ecsact_entity_id    entity_id,
	ecsact_component_id component_id,
	const void*         component_data,
	void*               user_data
) {
	std::cout //
		<< "[EVENT]: " << magic_enum::enum_name(event) << " "
		<< component_name(component_id) << "\n";
}

std::atomic_bool wasm_trap_happened = false;

void trap_handler(ecsact_system_like_id system_id, const char* trap_message) {
	wasm_trap_happened = true;
	std::string msg = "[WASM TRAP]: ";
	msg += system_name(system_id) + " triggered a trap: ";
	msg += std::string(trap_message);
	msg += "\n";

	std::cerr << msg;
	std::cerr.flush();
}

auto load_wasm_files(const std::vector<std::string>& wasm_file_paths) {
	using namespace std::string_view_literals;

	for(auto& wasm_path : wasm_file_paths) {
		auto err = ecsact::si::wasm::load_file(wasm_path, std::span{test_systems});

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
	std::ios_base::sync_with_stdio(false);

	auto wasm_file_paths = parse_args(argc, argv);

	ecsact::core::registry test_registry("Test Registry");

	auto test_entity = test_registry.create_entity();
	test_registry.add_component(test_entity, example::Spawner{});
	test_registry.add_component(test_entity, example::ExampleComponent{});

	ecsact::si::wasm::set_trap_handler(&trap_handler);
	load_wasm_files(wasm_file_paths);
	ecsact::si::wasm::consume_and_print_logs();

	for(auto i = 0; 200 > i; ++i) {
		auto para_entity = test_registry.create_entity();
		test_registry.add_component(
			para_entity,
			example::ExampleParallelComponent{}
		);
	}

	for(int i = 0; 10 > i; ++i) {
		std::cout << "\n==== EXECUTION (" << i << ") ====\n";

		ecsact_execution_events_collector ev_collector{
			.init_callback = &print_event,
			.init_callback_user_data = nullptr,
			.update_callback = &print_event,
			.update_callback_user_data = nullptr,
			.remove_callback = &print_event,
			.remove_callback_user_data = nullptr,
		};

		ecsact_execute_systems(test_registry.id(), 1, nullptr, &ev_collector);
		ecsact::si::wasm::consume_and_print_logs();

		std::cout //
			<< "[POST-EXECUTE]: Entity Count="
			<< ecsact_count_entities(test_registry.id()) << "\n";
	}

	std::cout << "\n (( Done ))\n";

	if(wasm_trap_happened) {
		return 1;
	}

	return 0;
}
