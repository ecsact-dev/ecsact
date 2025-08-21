#pragma once

#include "ecsact/runtime/meta.hh"
#include "ecsact/codegen/plugin.hh"
#include "rt_entt_codegen/shared/ecsact_entt_details.hh"

namespace ecsact::rt_entt_codegen::core {

auto print_parallel_system_execute(
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void;

auto print_execute_systems( //
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void;

auto print_execution_options( //
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void;

auto print_init_registry_storage( //
	ecsact::codegen_plugin_context&                     ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_details& details
) -> void;

auto print_execute_system_like_template_specializations( //
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void;

auto print_check_error_template_specializations( //
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void;

auto print_trigger_ecsact_events_minimal( //
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void;

auto print_trigger_ecsact_events_all( //
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void;

auto print_cleanup_ecsact_component_events( //
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void;

auto print_entity_sorting_components( //
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void;

auto print_system_marker_add_fn(
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void;

auto print_system_marker_remove_fn(
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void;

auto print_add_sys_beforestorage_fn(
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void;

auto print_cleanup_system_notifies(
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void;

auto print_entity_match_fn(
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void;

auto print_update_all_beforechange_storage(
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void;

auto print_apply_streaming_data(
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void;

auto print_copy_components(
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void;

auto print_hash_registry(
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void;

} // namespace ecsact::rt_entt_codegen::core
