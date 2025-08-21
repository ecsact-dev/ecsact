#include "ecsact/si/wasm.h"

#include <map>
#include <unordered_map>
#include <mutex>
#include <variant>
#include <vector>
#include <string>
#include <optional>
#include <string_view>
#include <shared_mutex>
#include <cassert>
#include <memory>
#include <wasm.h>
#include <wasmer.h>
#include <cstdlib>
#include <cstdio>
#include <functional>
#include <iostream>
#include <array>
#include <cstddef>
#include <thread>
#include "ecsact/runtime/dynamic.h"
#include "ecsact/si/wasmer/detail/minst/minst.hh"
#include "ecsact/si/wasmer/detail/logger.hh"
#include "ecsact/si/wasmer/detail/wasi_fs.hh"
#include "ecsact/si/wasmer/detail/globals.hh"
#include "ecsact/si/wasmer/detail/guest_imports/wasi_snapshot_preview1.hh"
#include "ecsact/si/wasmer/detail/guest_imports/env.hh"
#include "ecsact/si/wasmer/detail/cpp_util.hh"
#include "ecsact/si/wasmer/detail/mem_stack.hh"

using namespace std::string_literals;
using ecsact::wasm::detail::call_mem_alloc;
using ecsact::wasm::detail::clear_log_lines;
using ecsact::wasm::detail::consume_stdio_str_as_log_lines;
using ecsact::wasm::detail::get_log_lines;
using ecsact::wasm::detail::guest_env_module_imports;
using ecsact::wasm::detail::guest_wasi_module_imports;
using ecsact::wasm::detail::minst;
using ecsact::wasm::detail::minst_error;
using ecsact::wasm::detail::minst_export;
using ecsact::wasm::detail::minst_import;
using ecsact::wasm::detail::minst_import_resolve_t;
using ecsact::wasm::detail::set_call_mem_data;
using ecsact::wasm::detail::start_transaction;

namespace {
std::string last_error_message = "";

struct minst_ecsact_system_impls {
	minst                                                   minst;
	std::unordered_map<ecsact_system_like_id, minst_export> sys_impl_exports;
	minst_export                                            memory;

	minst_ecsact_system_impls() = delete;
	minst_ecsact_system_impls(const minst_ecsact_system_impls&) = delete;

	minst_ecsact_system_impls(minst_ecsact_system_impls&&) = default;

	minst_ecsact_system_impls( //
		class minst&&                                           minst,
		std::unordered_map<ecsact_system_like_id, minst_export> exports,
		minst_export                                            memory
	)
		: minst(std::move(minst))
		, sys_impl_exports(std::move(exports))
		, memory(memory) {
	}
};

auto trap_handler = ecsact_si_wasm_trap_handler{};

auto all_minsts = std::vector<std::shared_ptr<minst_ecsact_system_impls>>{};
auto next_available_minst_index = std::atomic_size_t{};

thread_local auto thread_minst = std::weak_ptr<minst_ecsact_system_impls>{};

auto ensure_minst() -> std::shared_ptr<minst_ecsact_system_impls> {
	auto minst = thread_minst.lock();
	if(!minst) {
		auto index = ++next_available_minst_index % all_minsts.size();
		minst = all_minsts[index];
		thread_minst = minst;
	}

	return minst;
}

void ecsact_si_wasm_system_impl(ecsact_system_execution_context* ctx) {
	auto minst = ensure_minst();
	auto system_id = ecsact_system_execution_context_id(ctx);
	auto itr = minst->sys_impl_exports.find(system_id);
	assert(itr != minst->sys_impl_exports.end());

	auto mem_data = std::array<std::byte, 4096>{};
	set_call_mem_data(mem_data.data(), mem_data.size());
	defer {
		set_call_mem_data(nullptr, 0);
	};
	call_mem_alloc(minst->memory.memory);
	itr->second.func_call(call_mem_alloc(ctx));
}

auto get_system_impl_exports(
	minst&                                                   inst,
	int                                                      systems_count,
	ecsact_system_like_id*                                   system_ids,
	const char**                                             wasm_exports,
	std::unordered_map<ecsact_system_like_id, minst_export>& system_impl_exports
) -> ecsact_si_wasm_error {
	system_impl_exports.clear();
	system_impl_exports.reserve(systems_count);

	for(auto i = 0; systems_count > i; ++i) {
		auto sys_id = system_ids[i];
		auto system_impl_export_name = std::string_view{
			wasm_exports[i],
			std::strlen(wasm_exports[i]),
		};

		auto exp = inst.find_export(system_impl_export_name);

		if(!exp) {
			return ECSACT_SI_WASM_ERR_EXPORT_NOT_FOUND;
		}

		if(exp->kind() != WASM_EXTERN_FUNC) {
			return ECSACT_SI_WASM_ERR_EXPORT_INVALID;
		}

		system_impl_exports[sys_id] = *exp;
	}

	return ECSACT_SI_WASM_OK;
}
} // namespace

void ecsact_si_wasm_last_error_message(
	char*   out_message,
	int32_t message_max_length
) {
	std::copy_n(
		last_error_message.begin(),
		std::min(
			message_max_length,
			static_cast<int32_t>(last_error_message.size())
		),
		out_message
	);
}

int32_t ecsact_si_wasm_last_error_message_length() {
	return static_cast<int32_t>(last_error_message.size());
}

ecsact_si_wasm_error ecsact_si_wasm_load(
	char*                  wasm_data,
	int                    wasm_data_size,
	int                    systems_count,
	ecsact_system_like_id* system_ids,
	const char**           wasm_exports
) {
	using ecsact::wasm::detail::engine;
	using ecsact::wasm::detail::minst;
	using ecsact::wasm::detail::minst_error_code;
	using ecsact::wasm::detail::minst_export;

#ifdef ECSACT_DYNAMIC_API_LOAD_AT_RUNTIME
	if(ecsact_set_system_execution_impl == nullptr) {
		return ECSACT_SI_WASM_ERR_NO_SET_SYSTEM_EXECUTION;
	}
#endif
	auto import_resolver = [&](const minst_import imp) -> minst_import_resolve_t {
		auto method_name = imp.name();

		if(imp.module() == "env") {
			auto itr = guest_env_module_imports.find(method_name);
			if(itr == guest_env_module_imports.end()) {
				return std::nullopt;
			}
			return itr->second();
		}

		if(imp.module() == "wasi_snapshot_preview1") {
			auto itr = guest_wasi_module_imports.find(method_name);
			if(itr == guest_wasi_module_imports.end()) {
				return std::nullopt;
			}
			return itr->second();
		}

		return std::nullopt;
	};

	all_minsts.clear();
	all_minsts.reserve(100);

	for(auto i = 0; 100 > i; ++i) {
		auto result = minst::create(
			engine(),
			std::span{
				reinterpret_cast<std::byte*>(wasm_data),
				static_cast<size_t>(wasm_data_size),
			},
			import_resolver
		);

		if(std::holds_alternative<minst_error>(result)) {
			auto err = std::get<minst_error>(result);
			last_error_message = err.message;
			switch(err.code) {
				case minst_error_code::ok:
					assert(err.code != minst_error_code::ok);
				case minst_error_code::compile_fail:
					return ECSACT_SI_WASM_ERR_COMPILE_FAIL;
				case minst_error_code::unresolved_guest_import:
					return ECSACT_SI_WASM_ERR_GUEST_IMPORT_INVALID;
				case minst_error_code::instantiate_fail:
					return ECSACT_SI_WASM_ERR_INSTANTIATE_FAIL;
			}
		}

		auto& inst = std::get<minst>(result);
		auto  system_impl_exports =
			std::unordered_map<ecsact_system_like_id, minst_export>{};

		auto err = get_system_impl_exports(
			inst,
			systems_count,
			system_ids,
			wasm_exports,
			system_impl_exports
		);

		if(err != ECSACT_SI_WASM_OK) {
			return err;
		}

		auto wasm_mem = std::optional<minst_export>{};

		for(auto exp : inst.exports()) {
			if(exp.kind() == WASM_EXTERN_MEMORY) {
				wasm_mem = exp;
				break;
			}
		}

		assert(wasm_mem);

		auto mem_data = std::array<std::byte, 4096>{};
		set_call_mem_data(mem_data.data(), mem_data.size());
		call_mem_alloc<wasm_memory_t*>(wasm_mem->memory);
		defer {
			set_call_mem_data(nullptr, 0);
		};
		auto init_trap = inst.initialize();
		if(init_trap) {
			return ECSACT_SI_WASM_ERR_INITIALIZE_FAIL;
		}

		all_minsts.emplace_back(std::make_shared<minst_ecsact_system_impls>( //
			std::move(inst),
			system_impl_exports,
			*wasm_mem
		));
	}

	for(auto i = 0; systems_count > i; ++i) {
		ecsact_set_system_execution_impl(
			system_ids[i],
			&ecsact_si_wasm_system_impl
		);
	}

	return ECSACT_SI_WASM_OK;
}

ecsact_si_wasm_error ecsact_si_wasm_load_file(
	const char*            wasm_file_path,
	int                    systems_count,
	ecsact_system_like_id* system_ids,
	const char**           wasm_exports
) {
	FILE* file = std::fopen(wasm_file_path, "rb");
	if(!file) {
		return ECSACT_SI_WASM_ERR_FILE_OPEN_FAIL;
	}

	auto binary_data = std::make_shared<std::vector<std::byte>>();

	std::fseek(file, 0L, SEEK_END);
	auto file_size = std::ftell(file);
	std::fseek(file, 0L, SEEK_SET);
	binary_data->resize(file_size);

	if(std::fread(binary_data->data(), file_size, 1, file) != 1) {
		return ECSACT_SI_WASM_ERR_FILE_READ_FAIL;
	}

	return ecsact_si_wasm_load(
		reinterpret_cast<char*>(binary_data->data()),
		static_cast<int>(binary_data->size()),
		systems_count,
		system_ids,
		wasm_exports
	);
}

void ecsact_si_wasm_set_trap_handler(ecsact_si_wasm_trap_handler handler) {
	trap_handler = handler;
}

void ecsact_si_wasm_unload(
	int                    systems_count,
	ecsact_system_like_id* system_ids
) {
}

void ecsact_si_wasm_reset() {
	all_minsts.clear();
	next_available_minst_index = 0;
}

void ecsact_si_wasm_consume_logs(
	ecsact_si_wasm_log_consumer consumer,
	void*                       consumer_user_data
) {
	auto t = start_transaction();
	for(auto& entry : get_log_lines(t)) {
		consumer(
			entry.log_level,
			entry.message.c_str(),
			static_cast<int32_t>(entry.message.size()),
			consumer_user_data
		);
	}

	for(auto& entry : consume_stdio_str_as_log_lines(t)) {
		consumer(
			entry.log_level,
			entry.message.c_str(),
			static_cast<int32_t>(entry.message.size()),
			consumer_user_data
		);
	}

	clear_log_lines(t);
}

int32_t ecsact_si_wasm_allow_file_read_access(
	const char* real_file_path,
	int32_t     real_file_path_length,
	const char* virtual_file_path,
	int32_t     virtual_file_path_length
) {
	return ecsact::wasm::detail::wasi::fs::allow_file_read_access(
		std::string_view{
			real_file_path,
			static_cast<size_t>(real_file_path_length),
		},
		std::string_view{
			virtual_file_path,
			static_cast<size_t>(virtual_file_path_length),
		}
	);
}
