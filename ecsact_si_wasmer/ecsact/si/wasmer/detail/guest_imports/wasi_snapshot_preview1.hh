#pragma once

#include "ecsact/si/wasmer/detail/guest_imports.hh"
#include "ecsact/si/wasmer/detail/util.hh"
#include "ecsact/si/wasmer/detail/wasi.hh"

namespace ecsact::wasm::detail {
const auto guest_wasi_module_imports = allowed_guest_imports_t{
	{
		"proc_exit",
		[]() -> minst_import_resolve_func {
			return {
				wasm_functype_new_1_0( //
					wasm_valtype_new(WASM_I32) // exit_code
				),
				&ecsact_si_wasi_proc_exit,
			};
		},
	},
	{
		"fd_seek",
		[]() -> minst_import_resolve_func {
			return {
				wasm_functype_new_4_1(
					wasm_valtype_new_i32(), // fd
					wasm_valtype_new_i64(), // offset
					wasm_valtype_new_i32(), // whence
					wasm_valtype_new_i32(), // retptr0
					wasm_valtype_new_i32() // error code (return)
				),
				&ecsact_si_wasi_fd_seek,
			};
		},
	},
	{
		"fd_write",
		[]() -> minst_import_resolve_func {
			return {
				wasm_functype_new_4_1(
					wasm_valtype_new_i32(), // fd
					wasm_valtype_new_i32(), // iovs
					wasm_valtype_new_i32(), // iovs_len
					wasm_valtype_new_i32(), // retptr0
					wasm_valtype_new_i32() // error code (return)
				),
				&ecsact_si_wasi_fd_write,
			};
		},
	},
	{
		"fd_read",
		[]() -> minst_import_resolve_func {
			return {
				wasm_functype_new_4_1(
					wasm_valtype_new_i32(), // fd
					wasm_valtype_new_i32(), // iovs
					wasm_valtype_new_i32(), // iovs_len
					wasm_valtype_new_i32(), // retptr0
					wasm_valtype_new_i32() // error code (return)
				),
				&ecsact_si_wasi_fd_read,
			};
		},
	},
	{
		"fd_close",
		[]() -> minst_import_resolve_func {
			return {
				wasm_functype_new_1_1(
					wasm_valtype_new_i32(), // fd
					wasm_valtype_new_i32() // error code (return)
				),
				&ecsact_si_wasi_fd_close,
			};
		},
	},
	{
		"environ_sizes_get",
		[]() -> minst_import_resolve_func {
			return {
				wasm_functype_new_2_1(
					wasm_valtype_new_i32(), // retptr0
					wasm_valtype_new_i32(), // retptr1
					wasm_valtype_new_i32() // error code (return)
				),
				&ecsact_si_wasi_environ_sizes_get,
			};
		},
	},
	{
		"environ_get",
		[]() -> minst_import_resolve_func {
			return {
				wasm_functype_new_2_1(
					wasm_valtype_new_i32(), // environ
					wasm_valtype_new_i32(), // environ_buf
					wasm_valtype_new_i32() // error code (return)
				),
				&ecsact_si_wasi_environ_get,
			};
		},
	},
	{
		"fd_fdstat_get",
		[]() -> minst_import_resolve_func {
			return {
				wasm_functype_new_2_1(
					wasm_valtype_new_i32(), // fd
					wasm_valtype_new_i32(), // retptr0
					wasm_valtype_new_i32() // error code (return)
				),
				&ecsact_si_wasi_fd_fdstat_get,
			};
		},
	},
};

} // namespace ecsact::wasm::detail
