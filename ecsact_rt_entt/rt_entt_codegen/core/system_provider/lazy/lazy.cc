#include "lazy.hh"

#include <format>

#include "ecsact/lang-support/lang-cc.hh"
#include "rt_entt_codegen/shared/sorting.hh"
#include "rt_entt_codegen/shared/util.hh"
#include "ecsact/runtime/meta.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"

using ecsact::cc_lang_support::cpp_identifier;
using ecsact::meta::decl_full_name;

using namespace ecsact::rt_entt_codegen::core;

auto provider::lazy::initialization(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> void {
	auto system_name = cpp_identifier(decl_full_name(sys_like_id));

	if(sys_like_id.is_system()) {
		lazy_iteration_rate =
			ecsact_meta_get_lazy_iteration_rate(sys_like_id.as_system());
	}

	exec_start_label_name = std::format(
		"exec_start_{}_",
		static_cast<int>(sys_like_id.get_sys_like_id())
	);

	pending_lazy_exec_struct = std::format(
		"::ecsact::entt::detail::pending_lazy_execution<::{}>",
		system_name
	);

	system_sorting_struct_name =
		std::format("::ecsact::entt::detail::system_sorted<{}>", system_name);
}

auto provider::lazy::before_make_view_or_group(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names,
	std::vector<std::string>&       additional_view_components
) -> void {
	ctx.write(
		"constexpr auto lazy_iteration_rate_ = ",
		lazy_iteration_rate,
		";\n\n"
	);
	ctx.write("auto iteration_count_ = 0;\n\n");
	ctx.write(exec_start_label_name, ":\n");
	additional_view_components.push_back(pending_lazy_exec_struct);

	if(sys_like_id.is_system()) {
		if(system_needs_sorted_entities(sys_like_id.as_system())) {
			additional_view_components.push_back(system_sorting_struct_name);
		}
	}
}

auto provider::lazy::after_make_view_or_group(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> void {
	if(sys_like_id.is_system()) {
		if(system_needs_sorted_entities(sys_like_id.as_system())) {
			ctx.write("view.use<", system_sorting_struct_name, ">();\n");
		}
	}
}

auto provider::lazy::pre_exec_system_impl(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> void {
	using ecsact::cpp_codegen_plugin_util::block;

	block(ctx, "if(iteration_count_ == lazy_iteration_rate_)", [&] {
		ctx.write("break;\n");
	});

	ctx.write("++iteration_count_;\n");
	ctx.write(
		names.registry_var_name,
		".erase<",
		pending_lazy_exec_struct,
		">(entity);\n"
	);
}

auto provider::lazy::post_iteration(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names
) -> void {
	using ecsact::cpp_codegen_plugin_util::block;

	auto system_name = cpp_identifier(decl_full_name(sys_like_id));

	ctx.write(
		"// If this assertion triggers that's a ecsact_rt_entt codegen "
		"failure\n"
	);
	ctx.write("assert(iteration_count_ <= lazy_iteration_rate_);\n");
	block(ctx, "if(iteration_count_ < lazy_iteration_rate_)", [&] {
		ctx.write(
			"_recalc_sorting_hash<",
			system_name,
			">(",
			names.registry_var_name,
			");\n"
		);
		ctx.write(
			names.registry_var_name,
			".sort<",
			system_sorting_struct_name,
			">([](const auto& a, const auto& b) { return a.hash < b.hash; });\n"
		);

		ecsact::rt_entt_codegen::util::make_view(
			ctx,
			"view_no_pending_lazy_",
			names.registry_var_name,
			system_details
		);

		ctx.write("auto view_no_pending_lazy_count_ = 0;\n");

		block(
			ctx,
			"for(ecsact::entt::entity_id entity : view_no_pending_lazy_)",
			[&] {
				ctx.write(
					"// If this assertion triggers this is an indicator of a codegen "
					"failure.\n"
					"// Please report to "
					"https://github.com/ecsact-dev/ecsact_rt_entt\n"
				);
				ctx.write(
					"assert(",
					names.registry_var_name,
					".all_of<",
					system_sorting_struct_name,
					">(entity));\n"
				);
				ctx.write("view_no_pending_lazy_count_ += 1;\n");
				ctx.write(
					names.registry_var_name,
					".emplace<",
					pending_lazy_exec_struct,
					">(entity);\n"
				);
			}
		);

		block(ctx, "if(view_no_pending_lazy_count_ >= lazy_iteration_rate_)", [&] {
			ctx.write("goto ", exec_start_label_name, ";\n");
		});
	});
}
