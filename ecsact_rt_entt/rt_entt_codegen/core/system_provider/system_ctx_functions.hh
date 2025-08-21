#pragma once

#include "rt_entt_codegen/core/sys_exec/sys_exec.hh"
#include "rt_entt_codegen/shared/ecsact_entt_details.hh"
#include "rt_entt_codegen/shared/system_variant.hh"

namespace ecsact::rt_entt_codegen::core::provider {
using capability_t =
	std::unordered_map<ecsact_component_like_id, ecsact_system_capability>;

auto context_action_impl(
	ecsact::codegen_plugin_context& ctx,
	const system_like_id_variant&   sys_like_id
) -> void;
auto context_add_impl(
	ecsact::codegen_plugin_context& ctx,
	const capability_t&             sys_caps
) -> void;
auto context_remove_impl(
	ecsact::codegen_plugin_context&                            ctx,
	const capability_t&                                        sys_caps,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	const std::string&                                         view_type_name
) -> void;
auto context_get_impl(
	ecsact::codegen_plugin_context&                            ctx,
	const system_like_id_variant&                              sys_like_id,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	const std::string&                                         view_type_name
) -> void;
auto context_update_impl(
	ecsact::codegen_plugin_context&                            ctx,
	const system_like_id_variant&                              sys_like_id,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	const std::string&                                         view_type_name
) -> void;
auto context_has_impl(
	ecsact::codegen_plugin_context&                            ctx,
	const system_like_id_variant&                              sys_like_id,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
) -> void;
auto context_generate_impl(
	ecsact::codegen_plugin_context&                            ctx,
	const system_like_id_variant&                              sys_like_id,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
) -> void;
auto context_stream_toggle_impl(
	ecsact::codegen_plugin_context&                            ctx,
	const system_like_id_variant&                              sys_like_id,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
) -> void;
auto context_parent_impl(
	ecsact::codegen_plugin_context& ctx,
	const system_like_id_variant&   sys_like_id
) -> void;
auto context_other_impl(
	ecsact::codegen_plugin_context&                            ctx,
	const system_like_id_variant&                              sys_like_id,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details
) -> void;
} // namespace ecsact::rt_entt_codegen::core::provider
