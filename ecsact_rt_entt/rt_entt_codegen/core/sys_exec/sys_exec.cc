#include "sys_exec.hh"

#include "rt_entt_codegen/shared/ecsact_entt_details.hh"
#include "ecsact/lang-support/lang-cc.hh"
#include "ecsact/runtime/meta.hh"
#include "rt_entt_codegen/shared/parallel.hh"

auto ecsact::rt_entt_codegen::core::print_child_systems(
	ecsact::codegen_plugin_context&                        ctx,
	const ecsact::rt_entt_codegen::core::common_vars&      names,
	const ecsact::rt_entt_codegen::system_like_id_variant& sys_like_id

) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::meta::decl_full_name;
	using ecsact::meta::get_child_system_ids;
	using ecsact::rt_entt_codegen::ecsact_entt_system_details;
	using ecsact::rt_entt_codegen::system_like_id_variant;

	auto child_system_ids = ecsact::meta::get_child_system_ids(sys_like_id);

	if(child_system_ids.size() == 1) {
		// TODO(Kelwan): Make use case system agnostic when we support
		// nested Action systems
		// Issue: https://github.com/ecsact-dev/ecsact_parse/issues/154
		for(auto child_sys_id : get_child_system_ids(sys_like_id)) {
			auto child_details = ecsact_entt_system_details::from_system_like(
				ecsact_id_cast<ecsact_system_like_id>(child_sys_id)
			);

			auto child_system_name = cpp_identifier(decl_full_name(child_sys_id));

			ctx.write(
				"ecsact::entt::execute_system<::" + child_system_name + ">(",
				names.registry_var_name,
				", &context, {});\n"
			);
		}
	} else {
		auto parallel_system_cluster =
			ecsact::rt_entt_codegen::parallel::get_parallel_execution_cluster(
				ctx,
				std::vector<system_like_id_variant>{
					child_system_ids.begin(),
					child_system_ids.end()
				}
			);

		ecsact::rt_entt_codegen::parallel::print_parallel_execution_cluster(
			ctx,
			parallel_system_cluster
		);
	}

	ctx.write("\n");
}
