/**
 * @file
 * @internal
 */

#ifndef WASM_ECSACT_SYSTEM_EXECUTION__H
#define WASM_ECSACT_SYSTEM_EXECUTION__H

#include <string>
#include <wasm.h>
#include "ecsact/runtime/common.h"

wasm_trap_t* wasm_ecsact_system_execution_context_action(
	const wasm_val_vec_t* args,
	wasm_val_vec_t*       results
);

wasm_trap_t* wasm_ecsact_system_execution_context_add(
	const wasm_val_vec_t* args,
	wasm_val_vec_t*       results
);

wasm_trap_t* wasm_ecsact_system_execution_context_remove(
	const wasm_val_vec_t* args,
	wasm_val_vec_t*       results
);

wasm_trap_t* wasm_ecsact_system_execution_context_get(
	const wasm_val_vec_t* args,
	wasm_val_vec_t*       results
);

wasm_trap_t* wasm_ecsact_system_execution_context_update(
	const wasm_val_vec_t* args,
	wasm_val_vec_t*       results
);

wasm_trap_t* wasm_ecsact_system_execution_context_has(
	const wasm_val_vec_t* args,
	wasm_val_vec_t*       results
);

wasm_trap_t* wasm_ecsact_system_execution_context_generate(
	const wasm_val_vec_t* args,
	wasm_val_vec_t*       results
);

wasm_trap_t* wasm_ecsact_system_execution_context_parent(
	const wasm_val_vec_t* args,
	wasm_val_vec_t*       results
);

wasm_trap_t* wasm_ecsact_system_execution_context_same(
	const wasm_val_vec_t* args,
	wasm_val_vec_t*       results
);

wasm_trap_t* wasm_ecsact_system_execution_context_other(
	const wasm_val_vec_t* args,
	wasm_val_vec_t*       results
);

wasm_trap_t* wasm_ecsact_system_execution_context_entity(
	const wasm_val_vec_t* args,
	wasm_val_vec_t*       results
);

wasm_trap_t* wasm_ecsact_system_execution_context_stream_toggle(
	const wasm_val_vec_t* args,
	wasm_val_vec_t*       results
);

#endif // WASM_ECSACT_SYSTEM_EXECUTION__H
