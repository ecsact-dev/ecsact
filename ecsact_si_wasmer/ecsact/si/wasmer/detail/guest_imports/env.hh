#pragma once

#include "ecsact/si/wasmer/detail/guest_imports.hh"
#include "ecsact/si/wasmer/detail/util.hh"
#include "ecsact/si/wasmer/detail/wasm_ecsact_system_execution.h"

namespace ecsact::wasm::detail {

const auto guest_env_module_imports = allowed_guest_imports_t{
	{
		"ecsact_system_execution_context_action",
		[]() -> minst_import_resolve_func {
			return {
				wasm_functype_new_2_0(
					wasm_valtype_new(WASM_I32), // context
					wasm_valtype_new(WASM_I32) // out_action_data
				),
				&wasm_ecsact_system_execution_context_action,
			};
		},
	},
	{
		"ecsact_system_execution_context_parent",
		[]() -> minst_import_resolve_func {
			return {
				wasm_functype_new_1_1(
					wasm_valtype_new(WASM_I32), // context
					wasm_valtype_new(WASM_I32) // parent context (return)
				),
				&wasm_ecsact_system_execution_context_parent,
			};
		},
	},
	{
		"ecsact_system_execution_context_same",
		[]() -> minst_import_resolve_func {
			return {
				wasm_functype_new_2_1(
					wasm_valtype_new(WASM_I32), // context a
					wasm_valtype_new(WASM_I32), // context b
					wasm_valtype_new(WASM_I32) // same bool (return)
				),
				&wasm_ecsact_system_execution_context_same,
			};
		},
	},
	{
		"ecsact_system_execution_context_get",
		[]() -> minst_import_resolve_func {
			return {
				wasm_functype_new_4_0(
					wasm_valtype_new(WASM_I32), // context
					wasm_valtype_new(WASM_I32), // component_id
					wasm_valtype_new(WASM_I32), // out_component_data
					wasm_valtype_new(WASM_I32) // indexed_fields
				),
				&wasm_ecsact_system_execution_context_get,
			};
		},
	},
	{
		"ecsact_system_execution_context_update",
		[]() -> minst_import_resolve_func {
			return {
				wasm_functype_new_4_0(
					wasm_valtype_new(WASM_I32), // context
					wasm_valtype_new(WASM_I32), // component_id
					wasm_valtype_new(WASM_I32), // component_data
					wasm_valtype_new(WASM_I32) // indexed_fields
				),
				&wasm_ecsact_system_execution_context_update,
			};
		},
	},
	{
		"ecsact_system_execution_context_has",
		[]() -> minst_import_resolve_func {
			return {
				wasm_functype_new_3_0(
					wasm_valtype_new(WASM_I32), // context
					wasm_valtype_new(WASM_I32), // component_id
					wasm_valtype_new(WASM_I32) // indexed_fields
				),
				&wasm_ecsact_system_execution_context_has,
			};
		},
	},
	{
		"ecsact_system_execution_context_generate",
		[]() -> minst_import_resolve_func {
			return {
				wasm_functype_new_4_0(
					wasm_valtype_new(WASM_I32), // context
					wasm_valtype_new(WASM_I32), // component_count
					wasm_valtype_new(WASM_I32), // component_ids
					wasm_valtype_new(WASM_I32) // components_data
				),
				&wasm_ecsact_system_execution_context_generate,
			};
		},
	},
	{
		"ecsact_system_execution_context_add",
		[]() -> minst_import_resolve_func {
			return {
				wasm_functype_new_3_0(
					wasm_valtype_new(WASM_I32), // context
					wasm_valtype_new(WASM_I32), // component_id
					wasm_valtype_new(WASM_I32) // component_data
				),
				&wasm_ecsact_system_execution_context_add,
			};
		},
	},
	{
		"ecsact_system_execution_context_remove",
		[]() -> minst_import_resolve_func {
			return {
				wasm_functype_new_3_0(
					wasm_valtype_new(WASM_I32), // context
					wasm_valtype_new(WASM_I32), // component_id
					wasm_valtype_new(WASM_I32) // indexed_fields
				),
				&wasm_ecsact_system_execution_context_remove,
			};
		},
	},
	{
		"ecsact_system_execution_context_other",
		[]() -> minst_import_resolve_func {
			return {
				wasm_functype_new_2_1(
					wasm_valtype_new(WASM_I32), // context
					wasm_valtype_new(WASM_I32), // entity_id
					wasm_valtype_new(WASM_I32) // other context (return)
				),
				&wasm_ecsact_system_execution_context_other,
			};
		},
	},
	{
		"ecsact_system_execution_context_entity",
		[]() -> minst_import_resolve_func {
			return {
				wasm_functype_new_1_1(
					wasm_valtype_new(WASM_I32), // context
					wasm_valtype_new(WASM_I32) // entity 9return)
				),
				&wasm_ecsact_system_execution_context_entity,
			};
		},
	},
	{
		"ecsact_system_execution_context_stream_toggle",
		[]() -> minst_import_resolve_func {
			return {
				wasm_functype_new_4_0(
					wasm_valtype_new(WASM_I32), // context
					wasm_valtype_new(WASM_I32), // component_id
					wasm_valtype_new(WASM_I32), // streaming_enabled
					wasm_valtype_new(WASM_I32) // indexed_fields
				),
				&wasm_ecsact_system_execution_context_stream_toggle,
			};
		},
	},
};

} // namespace ecsact::wasm::detail
