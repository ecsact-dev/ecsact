#include "ecsact/si/wasmer/detail/wasi.hh"

#include <cstdio>
#include <map>
#include <string_view>
#include "ecsact/si/wasmer/detail/wasi_fs.hh"
#include "ecsact/si/wasmer/detail/logger.hh"
#include "ecsact/si/wasmer/detail/mem_stack.hh"
#include "ecsact/si/wasmer/detail/util.hh"

using ecsact::wasm::detail::call_mem_read;
using ecsact::wasm::detail::debug_trace_method;
using ecsact::wasm::detail::wasm_memory_cast;

constexpr int32_t WASI_STDIN_FD = 0;
constexpr int32_t WASI_STDOUT_FD = 1;
constexpr int32_t WASI_STDERR_FD = 2;

wasm_trap_t* ecsact_si_wasi_proc_exit(
	const wasm_val_vec_t* args,
	wasm_val_vec_t*       results
) {
	debug_trace_method("proc_exit");
	return nullptr;
}

wasm_trap_t* ecsact_si_wasi_fd_seek(
	const wasm_val_vec_t* args,
	wasm_val_vec_t*       results
) {
	debug_trace_method("fd_seek");
	results->data[0].kind = WASM_I32;
	results->data[0].of.i32 = 0;

	return nullptr;
}

wasm_trap_t* ecsact_si_wasi_fd_write(
	const wasm_val_vec_t* args,
	wasm_val_vec_t*       results
) {
	debug_trace_method("fd_write");
	auto mem = call_mem_read<wasm_memory_t*>(0);
	assert(args->data[0].kind == WASM_I32);
	auto fd = args->data[0].of.i32;

	assert(args->data[1].kind == WASM_I32);
	auto iovec =
		wasm_memory_cast<ecsact_si_wasi_ciovec_t>(mem, args->data[1].of.i32);

	assert(args->data[2].kind == WASM_I32);
	auto iovec_len = args->data[2].of.i32;

	assert(args->data[3].kind == WASM_I32);
	auto out_write_amount = wasm_memory_cast<size_t>(mem, args->data[3].of.i32);

	if(fd == WASI_STDERR_FD || fd == WASI_STDOUT_FD) {
		auto write_amount = size_t{};
		auto log_level = fd == WASI_STDOUT_FD //
			? ECSACT_SI_WASM_LOG_LEVEL_INFO
			: ECSACT_SI_WASM_LOG_LEVEL_ERROR;

		for(int i = 0; iovec_len > i; ++i) {
			auto io = iovec[i];

			if(io.buf_len > 0) {
				auto buf = wasm_memory_cast<const char>(mem, io.buf);
				auto str = std::string_view(buf, io.buf_len);
				write_amount += io.buf_len;
				ecsact::wasm::detail::push_stdio_str(log_level, str);
			}
		}

		*out_write_amount = write_amount;

		results->data[0].kind = WASM_I32;
		results->data[0].of.i32 = 0;
	} else {
		results->data[0].kind = WASM_I32;
		results->data[0].of.i32 = 1;
	}

	return nullptr;
}

wasm_trap_t* ecsact_si_wasi_fd_read(
	const wasm_val_vec_t* args,
	wasm_val_vec_t*       results
) {
	debug_trace_method("fd_read");
	auto mem = call_mem_read<wasm_memory_t*>(0);
	assert(args->data[0].kind == WASM_I32);
	auto fd = args->data[0].of.i32;

	assert(args->data[1].kind == WASM_I32);
	auto iovec =
		wasm_memory_cast<ecsact_si_wasi_ciovec_t>(mem, args->data[1].of.i32);

	assert(args->data[2].kind == WASM_I32);
	auto iovec_len = args->data[2].of.i32;

	assert(args->data[3].kind == WASM_I32);
	auto out_read_amount = wasm_memory_cast<size_t>(mem, args->data[3].of.i32);

	auto f = ecsact::wasm::detail::wasi::fs::ensure_open(fd);
	auto read_amount = size_t{};

	for(int i = 0; iovec_len > i; ++i) {
		auto io = iovec[i];

		if(io.buf_len > 0) {
			auto buf = wasm_memory_cast<char>(mem, io.buf);
			read_amount += std::fread(buf, 1, io.buf_len, f);
		}
	}

	*out_read_amount = read_amount;

	results->data[0].kind = WASM_I32;
	results->data[0].of.i32 = 0;

	return nullptr;
}

wasm_trap_t* ecsact_si_wasi_fd_close(
	const wasm_val_vec_t* args,
	wasm_val_vec_t*       results
) {
	debug_trace_method("fd_close");
	assert(args->data[0].kind == WASM_I32);
	auto fd = args->data[0].of.i32;

	if(fd != WASI_STDOUT_FD && fd != WASI_STDERR_FD && fd != WASI_STDIN_FD) {
		ecsact::wasm::detail::wasi::fs::close(fd);
		results->data[0].kind = WASM_I32;
		results->data[0].of.i32 = 0;
	} else {
		results->data[0].kind = WASM_I32;
		results->data[0].of.i32 = 1;
	}

	return nullptr;
}

wasm_trap_t* ecsact_si_wasi_environ_sizes_get(
	const wasm_val_vec_t* args,
	wasm_val_vec_t*       results
) {
	debug_trace_method("environ_sizes_get");
	auto mem = call_mem_read<wasm_memory_t*>(0);
	auto retptr0 = wasm_memory_cast<size_t>(mem, args->data[0].of.i32);
	auto retptr1 = wasm_memory_cast<size_t>(mem, args->data[1].of.i32);

	*retptr0 = 0;
	*retptr1 = 0;

	results->data[0].kind = WASM_I32;
	results->data[0].of.i32 = 0;

	return nullptr;
}

wasm_trap_t* ecsact_si_wasi_environ_get(
	const wasm_val_vec_t* args,
	wasm_val_vec_t*       results
) {
	debug_trace_method("environ_get");
	auto mem = call_mem_read<wasm_memory_t*>(0);
	auto environ_arg = wasm_memory_cast<uint8_t>(mem, args->data[0].of.i32);
	auto environ_buf_arg = wasm_memory_cast<uint8_t>(mem, args->data[1].of.i32);

	// Even though our environment variable size is `0` the API expects the key
	// values pairs to be null terminated.
	environ_arg[0] = '\0';
	environ_buf_arg[0] = '\0';

	results->data[0].kind = WASM_I32;
	results->data[0].of.i32 = 0;

	return nullptr;
}

wasm_trap_t* ecsact_si_wasi_fd_fdstat_get(
	const wasm_val_vec_t* args,
	wasm_val_vec_t*       results
) {
	debug_trace_method("fd_fdstat_get");
	const auto default_fdstats = std::map<int32_t, ecsact_si_wasi_fdstat_t>{
		{
			WASI_STDIN_FD,
			ecsact_si_wasi_fdstat_t{},
		},
		{
			WASI_STDOUT_FD,
			ecsact_si_wasi_fdstat_t{
				.fs_filetype = ecsact_si_wasi_filetype::character_device,
				.fs_flags = {},
				.fs_rights_base = ecsact_si_wasi_rights::fd_datasync |
					ecsact_si_wasi_rights::fd_sync | ecsact_si_wasi_rights::fd_write |
					ecsact_si_wasi_rights::fd_advise |
					ecsact_si_wasi_rights::fd_filestat_get |
					ecsact_si_wasi_rights::poll_fd_readwrite,
				.fs_rights_inheriting = {},
			},
		},
		{
			WASI_STDERR_FD,
			ecsact_si_wasi_fdstat_t{
				.fs_filetype = ecsact_si_wasi_filetype::character_device,
				.fs_flags = {},
				.fs_rights_base = ecsact_si_wasi_rights::fd_datasync |
					ecsact_si_wasi_rights::fd_sync | ecsact_si_wasi_rights::fd_write |
					ecsact_si_wasi_rights::fd_advise |
					ecsact_si_wasi_rights::fd_filestat_get |
					ecsact_si_wasi_rights::poll_fd_readwrite,
				.fs_rights_inheriting = {},
			},
		},
	};

	auto mem = call_mem_read<wasm_memory_t*>(0);

	assert(args->data[0].kind == WASM_I32);
	auto fd = args->data[0].of.i32;

	assert(args->data[1].kind == WASM_I32);
	auto ret =
		wasm_memory_cast<ecsact_si_wasi_fdstat_t>(mem, args->data[1].of.i32);

	if(default_fdstats.contains(fd)) {
		*ret = default_fdstats.at(fd);
	} else {
		*ret = ecsact::wasm::detail::wasi::fs::fdstat(fd);
	}

	if(ret->fs_filetype != ecsact_si_wasi_filetype::unknown) {
		results->data[0].kind = WASM_I32;
		results->data[0].of.i32 = 0;
	} else {
		results->data[0].kind = WASM_I32;
		results->data[0].of.i32 = 1;
	}

	return nullptr;
}
