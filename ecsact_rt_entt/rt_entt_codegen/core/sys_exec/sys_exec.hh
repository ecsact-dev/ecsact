#pragma once

#include <string>
#include <optional>

#include "rt_entt_codegen/shared/system_variant.hh"
#include "ecsact/codegen/plugin.hh"

namespace ecsact::rt_entt_codegen::core {

struct common_vars {
	std::string registry_var_name;
	std::string parent_context_var_name;
	/// only set if system is an action
	std::optional<std::string> action_var_name;
};

auto print_child_systems(
	ecsact::codegen_plugin_context&                   ctx,
	const ecsact::rt_entt_codegen::core::common_vars& names,
	const system_like_id_variant&                     sys_like_id
) -> void;
} // namespace ecsact::rt_entt_codegen::core
