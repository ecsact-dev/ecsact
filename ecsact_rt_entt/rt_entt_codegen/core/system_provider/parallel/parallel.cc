#include "parallel.hh"

#include <format>
#include "ecsact/lang-support/lang-cc.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"

using namespace ecsact::rt_entt_codegen::core;

auto provider::parallel::entity_iteration(
	ecsact::codegen_plugin_context&                   ctx,
	ecsact_system_like_id                             sys_like_id,
	const ecsact::rt_entt_codegen::core::common_vars& names,
	std::function<void()>                             iter_func
) -> handle_exclusive_provide {
	using ecsact::cpp_codegen_plugin_util::block;
	using namespace std::string_literals;
	using ecsact::meta::get_system_parallel_execution;

	block(ctx, "std::for_each(view.begin(), view.end(), [&](auto entity)", [&] {
		iter_func();
	});
	ctx.writef("{}", ");\n");
	return HANDLED;
}

auto provider::parallel::provide_context_init(
	ecsact::codegen_plugin_context&                   ctx,
	const ecsact::rt_entt_codegen::core::common_vars& names,
	std::string_view                                  context_type_name
) -> handle_exclusive_provide {
	return HANDLED;
}

auto provider::parallel::pre_exec_system_impl_context_init(
	ecsact::codegen_plugin_context&                   ctx,
	const ecsact::rt_entt_codegen::core::common_vars& names,
	std::string_view                                  context_type_name
) -> void {
	ctx.writef("{} context;\n\n", context_type_name);

	auto system_name =
		cc_lang_support::cpp_identifier(meta::decl_full_name(sys_like_id));

	ctx.writef("{}{}{}", "context.registry = &", names.registry_var_name, ";\n");
	if(names.action_var_name) {
		ctx.writef(
			"{}{}{}",
			"context.action_data = ",
			*names.action_var_name,
			";\n\n"
		);
	}

	ctx.writef(
		"{}{}{}",
		"context.id = ecsact_id_cast<ecsact_system_like_id>(::",
		system_name,
		"::id);\n"
	);
	ctx.writef(
		"{}{}{}",
		"context.parent_ctx = ",
		names.parent_context_var_name,
		";\n"
	);
	if(!names.parent_context_var_name.empty()) {
		ctx.writef(
			"context.execution_metadata = {0} ? {0}->execution_metadata : (registry.ctx().find<execution_metadata_wrapper>() ? registry.ctx().find<execution_metadata_wrapper>()->ptr : nullptr);\n",
			names.parent_context_var_name
		);
	} else {
		ctx.writef("context.execution_metadata = execution_metadata;\n");
	}
	ctx.writef("{}", "context.view = &view;\n\n");
	ctx.writef("{}", "context.entity = entity;\n");
}
