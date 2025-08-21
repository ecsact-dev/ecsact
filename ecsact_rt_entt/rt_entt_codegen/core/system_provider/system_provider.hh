#pragma once

#include <string_view>
#include <vector>
#include <string>
#include <functional>

#include "rt_entt_codegen/core/sys_exec/sys_exec.hh"
#include "rt_entt_codegen/shared/system_variant.hh"
#include "rt_entt_codegen/shared/ecsact_entt_details.hh"

namespace ecsact::rt_entt_codegen::core::provider {
enum handle_exclusive_provide {
	HANDLED = 0,
	NOT_HANDLED = 1,
};

class system_provider {
public:
	system_provider(system_like_id_variant sys_like_id);
	virtual ~system_provider();

	virtual auto initialization(
		ecsact::codegen_plugin_context&                   ctx,
		const ecsact::rt_entt_codegen::core::common_vars& names
	) -> void;

	virtual auto before_make_view_or_group(
		ecsact::codegen_plugin_context&                   ctx,
		const ecsact::rt_entt_codegen::core::common_vars& names,
		std::vector<std::string>&                         additional_view_components
	) -> void;

	virtual auto after_make_view_or_group(
		ecsact::codegen_plugin_context&                   ctx,
		const ecsact::rt_entt_codegen::core::common_vars& names
	) -> void;

	virtual auto context_function_header(
		ecsact::codegen_plugin_context&                   ctx,
		const ecsact::rt_entt_codegen::core::common_vars& names
	) -> void;

	[[nodiscard]] virtual auto context_function_action(
		ecsact::codegen_plugin_context&                   ctx,
		const ecsact::rt_entt_codegen::core::common_vars& names
	) -> handle_exclusive_provide;
	[[nodiscard]] virtual auto context_function_add(
		ecsact::codegen_plugin_context&                   ctx,
		const ecsact::rt_entt_codegen::core::common_vars& names
	) -> handle_exclusive_provide;
	[[nodiscard]] virtual auto context_function_remove(
		ecsact::codegen_plugin_context&                   ctx,
		const ecsact::rt_entt_codegen::core::common_vars& names
	) -> handle_exclusive_provide;
	[[nodiscard]] virtual auto context_function_get(
		ecsact::codegen_plugin_context&                   ctx,
		const ecsact::rt_entt_codegen::core::common_vars& names
	) -> handle_exclusive_provide;
	[[nodiscard]] virtual auto context_function_update(
		ecsact::codegen_plugin_context&                   ctx,
		const ecsact::rt_entt_codegen::core::common_vars& names
	) -> handle_exclusive_provide;
	[[nodiscard]] virtual auto context_function_has(
		ecsact::codegen_plugin_context&                   ctx,
		const ecsact::rt_entt_codegen::core::common_vars& names
	) -> handle_exclusive_provide;
	[[nodiscard]] virtual auto context_function_generate(
		ecsact::codegen_plugin_context&                   ctx,
		const ecsact::rt_entt_codegen::core::common_vars& names
	) -> handle_exclusive_provide;
	[[nodiscard]] virtual auto context_function_parent(
		ecsact::codegen_plugin_context&                   ctx,
		const ecsact::rt_entt_codegen::core::common_vars& names
	) -> handle_exclusive_provide;
	[[nodiscard]] virtual auto context_function_other(
		ecsact::codegen_plugin_context&                   ctx,
		const ecsact::rt_entt_codegen::core::common_vars& names
	) -> handle_exclusive_provide;
	[[nodiscard]] virtual auto context_function_stream_toggle(
		ecsact::codegen_plugin_context&                   ctx,
		const ecsact::rt_entt_codegen::core::common_vars& names
	) -> handle_exclusive_provide;

	virtual auto pre_entity_iteration(
		ecsact::codegen_plugin_context&                   ctx,
		const ecsact::rt_entt_codegen::core::common_vars& names
	) -> void;

	virtual auto provide_context_init(
		ecsact::codegen_plugin_context&                   ctx,
		const ecsact::rt_entt_codegen::core::common_vars& names,
		std::string_view                                  context_type_name
	) -> handle_exclusive_provide;

	[[nodiscard]] virtual auto entity_iteration(
		ecsact::codegen_plugin_context&                   ctx,
		ecsact_system_like_id                             sys_like_id,
		const ecsact::rt_entt_codegen::core::common_vars& names,
		std::function<void()>                             iter_func
	) -> handle_exclusive_provide;

	virtual auto pre_exec_system_impl(
		ecsact::codegen_plugin_context&                   ctx,
		const ecsact::rt_entt_codegen::core::common_vars& names
	) -> void;

	virtual auto pre_exec_system_impl_context_init(
		ecsact::codegen_plugin_context&                   ctx,
		const ecsact::rt_entt_codegen::core::common_vars& names,
		std::string_view                                  context_type_name
	) -> void;

	virtual auto system_impl(
		ecsact::codegen_plugin_context&                   ctx,
		const ecsact::rt_entt_codegen::core::common_vars& names
	) -> handle_exclusive_provide;

	virtual auto post_exec_system_impl(
		ecsact::codegen_plugin_context&                   ctx,
		const ecsact::rt_entt_codegen::core::common_vars& names
	) -> void;

	virtual auto post_iteration(
		ecsact::codegen_plugin_context&                   ctx,
		const ecsact::rt_entt_codegen::core::common_vars& names
	) -> void;

protected:
	const system_like_id_variant     sys_like_id;
	const ecsact_entt_system_details system_details;
};
} // namespace ecsact::rt_entt_codegen::core::provider
