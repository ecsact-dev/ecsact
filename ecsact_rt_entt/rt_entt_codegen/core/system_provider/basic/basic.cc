#include "basic.hh"

#include <format>
#include "ecsact/runtime/meta.hh"
#include "ecsact/lang-support/lang-cc.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"
#include "rt_entt_codegen/core/system_provider/system_ctx_functions.hh"

using namespace ecsact::rt_entt_codegen::core;

auto provider::basic::initialization(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> void {
	sys_caps = ecsact::meta::system_capabilities(sys_like_id);

	view_type_name = "view";
}

auto provider::basic::context_function_header(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> void {
	if(names.action_var_name) {
		ctx.writef("{}", "const void* action_data = nullptr;\n");
	}
}

auto provider::basic::context_function_action(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	context_action_impl(ctx, sys_like_id);
	return HANDLED;
}

auto provider::basic::context_function_add(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	auto sys_caps = ecsact::meta::system_capabilities(sys_like_id);

	context_add_impl(ctx, sys_caps);
	return HANDLED;
}

auto provider::basic::context_function_remove(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	auto sys_caps = ecsact::meta::system_capabilities(sys_like_id);

	context_remove_impl(ctx, sys_caps, system_details, view_type_name);
	return HANDLED;
}

auto provider::basic::context_function_get(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	context_get_impl(ctx, sys_like_id, system_details, view_type_name);
	return HANDLED;
}

auto provider::basic::context_function_update(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	context_update_impl(ctx, sys_like_id, system_details, view_type_name);
	return HANDLED;
}

auto provider::basic::context_function_has(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	context_has_impl(ctx, sys_like_id, system_details);
	return HANDLED;
}

auto provider::basic::context_function_generate(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	context_generate_impl(ctx, sys_like_id, system_details);
	return HANDLED;
}

auto provider::basic::context_function_parent(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	context_parent_impl(ctx, sys_like_id);
	return HANDLED;
}

auto provider::basic::context_function_other(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	ctx.writef("{}", "return nullptr;");
	return HANDLED;
}

auto provider::basic::context_function_stream_toggle(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	context_stream_toggle_impl(ctx, sys_like_id, system_details);
	return HANDLED;
}

auto provider::basic::system_impl(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> handle_exclusive_provide {
	ctx.writef("{}", "system_impl(&context);\n");
	return HANDLED;
}

auto provider::basic::entity_iteration(
	ecsact::codegen_plugin_context&                   ctx,
	ecsact_system_like_id                             sys_like_id,
	const ecsact::rt_entt_codegen::core::common_vars& names,
	std::function<void()>                             iter_func
) -> handle_exclusive_provide {
	using ecsact::cpp_codegen_plugin_util::block;

	block(ctx, "for(ecsact::entt::entity_id entity : view)", [&] {
		iter_func();
	});
	return HANDLED;
}

auto provider::basic::provide_context_init(
	ecsact::codegen_plugin_context&                   ctx,
	const ecsact::rt_entt_codegen::core::common_vars& names,
	std::string_view                                  context_type_name
) -> handle_exclusive_provide {
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
	return HANDLED;
}

auto provider::basic::pre_exec_system_impl_context_init(
	ecsact::codegen_plugin_context&                   ctx,
	const ecsact::rt_entt_codegen::core::common_vars& names,
	std::string_view                                  context_type_name
) -> void {
	ctx.writef("{}", "context.entity = entity;\n");
}
