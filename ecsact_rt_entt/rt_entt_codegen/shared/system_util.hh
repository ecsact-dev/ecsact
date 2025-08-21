#pragma once

#include "ecsact/runtime/meta.hh"
#include "ecsact/lang-support/lang-cc.hh"
#include "rt_entt_codegen/shared/system_variant.hh"

namespace ecsact::rt_entt_codegen::system_util {

using capability_t =
	std::unordered_map<ecsact_component_like_id, ecsact_system_capability>;

/*
 * Checks if a system uses notify and should implement the run_system<S>
 * component in its execution
 */
auto is_notify_system(ecsact_system_like_id system_id) -> bool;

auto is_trivial_system(ecsact_system_like_id system_id) -> bool;

auto get_unique_view_name() -> std::string;

template<typename ComponentLikeID>
static auto create_context_struct_name( //
	ComponentLikeID component_like_id
) -> std::string {
	using ecsact::cc_lang_support::c_identifier;
	auto full_name =
		c_identifier(ecsact::meta::decl_full_name(component_like_id));
	return full_name + "Struct";
}

template<typename ComponentLikeID>
static auto create_context_var_name( //
	ComponentLikeID component_like_id
) -> std::string {
	using ecsact::cc_lang_support::c_identifier;
	auto full_name =
		c_identifier(ecsact::meta::decl_full_name(component_like_id));
	return full_name + "_context";
}

} // namespace ecsact::rt_entt_codegen::system_util
