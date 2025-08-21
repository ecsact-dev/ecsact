#include "notify.hh"

#include <map>

#include "ecsact/lang-support/lang-cc.hh"
#include "rt_entt_codegen/shared/util.hh"
#include "rt_entt_codegen/shared/system_util.hh"
#include "ecsact/runtime/meta.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"

using namespace ecsact::rt_entt_codegen::core;

auto provider::notify::before_make_view_or_group(
	ecsact::codegen_plugin_context& ctx,
	const common_vars&              names,
	std::vector<std::string>&       additional_view_components
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::meta::decl_full_name;
	using ecsact::rt_entt_codegen::system_util::is_notify_system;

	auto system_name = cpp_identifier(decl_full_name(sys_like_id));

	if(is_notify_system(sys_like_id)) {
		additional_view_components.push_back(
			std::format("ecsact::entt::detail::run_system<{}>", system_name)
		);
		print_system_notify_views(ctx, sys_like_id, names.registry_var_name);
	}
}

auto provider::notify::print_system_notify_views(
	ecsact::codegen_plugin_context& ctx,
	ecsact_system_like_id           system_id,
	std::string                     registry_name
) -> void {
	using ecsact::cc_lang_support::c_identifier;
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::meta::decl_full_name;

	auto notify_settings = ecsact::meta::system_notify_settings(system_id);

	using notify_settings_pair_t =
		std::pair<ecsact_component_like_id, ecsact_system_notify_setting>;

	auto notify_settings_vec = std::vector<notify_settings_pair_t>(
		notify_settings.begin(),
		notify_settings.end()
	);

	const auto notify_settings_ranking =
		std::map<ecsact_system_notify_setting, int>{
			{ECSACT_SYS_NOTIFY_ALWAYS, 99}, // unused
			{ECSACT_SYS_NOTIFY_NONE, 99}, // unused

			// We prioritize everything except onchange due to onchange having the
			// most overhead.
			{ECSACT_SYS_NOTIFY_ONINIT, 10},
			{ECSACT_SYS_NOTIFY_ONREMOVE, 9},
			{ECSACT_SYS_NOTIFY_ONUPDATE, 8},
			{ECSACT_SYS_NOTIFY_ONCHANGE, 0},
		};

	std::sort(
		notify_settings_vec.begin(),
		notify_settings_vec.end(),
		[&](notify_settings_pair_t a, notify_settings_pair_t b) -> bool {
			return notify_settings_ranking.at(a.second) >
				notify_settings_ranking.at(b.second);
		}
	);

	auto system_name = cpp_identifier(decl_full_name(system_id));

	for(auto const& [comp_id, notify_setting] : notify_settings_vec) {
		if(notify_setting == ECSACT_SYS_NOTIFY_ALWAYS ||
			 notify_setting == ECSACT_SYS_NOTIFY_NONE) {
			break;
		}
		auto cpp_comp_name = cpp_identifier(decl_full_name(comp_id));
		auto comp_name = c_identifier(decl_full_name((comp_id)));

		auto run_system_comp =
			std::format("ecsact::entt::detail::run_system<{}>", system_name);

		if(notify_setting == ECSACT_SYS_NOTIFY_ONINIT) {
			auto pending_add_str =
				std::format("ecsact::entt::component_added<{}>", cpp_comp_name);

			auto view_name = std::format("{}_init_view", comp_name);

			ecsact::rt_entt_codegen::util::make_view(
				ctx,
				view_name,
				registry_name,
				system_details,
				std::vector{pending_add_str},
				std::vector{run_system_comp}
			);

			block(ctx, std::format("for(auto entity: {})", view_name), [&]() {
				ctx.write(
					std::format("registry.emplace<{}>(entity);\n", run_system_comp)
				);
			});
		}

		if(notify_setting == ECSACT_SYS_NOTIFY_ONREMOVE) {
			auto pending_remove_str =
				std::format("ecsact::entt::component_removed<{}>", cpp_comp_name);

			auto view_name = std::format("{}_remove_view", comp_name);

			ecsact::rt_entt_codegen::util::make_view(
				ctx,
				view_name,
				registry_name,
				system_details,
				std::vector{pending_remove_str},
				std::vector{run_system_comp}
			);

			block(ctx, std::format("for(auto entity: {})", view_name), [&]() {
				ctx.write(
					std::format("registry.emplace<{}>(entity);\n", run_system_comp)
				);
			});
		}

		if(notify_setting == ECSACT_SYS_NOTIFY_ONUPDATE) {
			auto view_name = std::format("{}_update_view", comp_name);

			ecsact::rt_entt_codegen::util::make_view(
				ctx,
				view_name,
				registry_name,
				system_details,
				{},
				std::vector{run_system_comp}
			);

			block(ctx, std::format("for(auto entity: {})", view_name), [&]() {
				ctx.write(
					std::format("registry.emplace<{}>(entity);\n", run_system_comp)
				);
			});
		}

		if(notify_setting == ECSACT_SYS_NOTIFY_ONCHANGE) {
			auto exec_itr_onchange_str = std::format(
				"ecsact::entt::detail::exec_itr_beforechange_storage<{}>",
				cpp_comp_name
			);

			auto view_name = std::format("{}_change_view", comp_name);

			ecsact::rt_entt_codegen::util::make_view(
				ctx,
				view_name,
				registry_name,
				system_details,
				std::vector{exec_itr_onchange_str},
				std::vector{run_system_comp}
			);

			block(ctx, std::format("for(auto entity: {})", view_name), [&]() {
				block(
					ctx,
					std::format(
						"if(!ecsact::entt::wrapper::core::has_component_changed<{}>(entity,"
						" registry))",
						cpp_comp_name
					),
					[&] { ctx.write("continue;\n"); }
				);

				ctx.write(
					std::format("registry.emplace<{}>(entity);\n", run_system_comp)
				);
			});
		}
	}
}
