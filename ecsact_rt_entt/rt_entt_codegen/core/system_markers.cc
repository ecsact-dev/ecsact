#include "rt_entt_codegen/core/core.hh"

#include <set>
#include <format>
#include "ecsact/runtime/meta.hh"
#include "ecsact/lang-support/lang-cc.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"
#include "rt_entt_codegen/shared/sorting.hh"
#include "rt_entt_codegen/shared/system_util.hh"

using ecsact::cpp_codegen_plugin_util::block;
using ecsact::cpp_codegen_plugin_util::method_printer;
using ecsact::rt_entt_codegen::system_needs_sorted_entities;

auto ecsact::rt_entt_codegen::core::print_system_marker_add_fn(
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void {
	for(auto comp : details.all_components) {
		auto comp_like_id = ecsact_id_cast<ecsact_component_like_id>(comp);
		auto comp_name = ecsact::meta::decl_full_name(comp);
		auto comp_cpp_ident = cc_lang_support::cpp_identifier(comp_name);
		auto sorting_structs_covered = std::set<ecsact_system_id>{};

		ctx.write("template<>\n");
		auto printer =
			method_printer{
				ctx,
				"ecsact::entt::detail::add_system_markers_if_needed<" + comp_cpp_ident +
					">"
			}
				.parameter("ecsact::entt::registry_t&", "reg")
				.parameter("ecsact::entt::entity_id", "entity")
				.return_type("void");

		for(auto system_id : details.all_systems) {
			if(sorting_structs_covered.contains(system_id)) {
				continue;
			}

			auto sys_details = ecsact_entt_system_details::from_system_like(
				ecsact_id_cast<ecsact_system_like_id>(system_id)
			);

			if(!sys_details.readable_comps.contains(comp_like_id)) {
				continue;
			}

			if(system_needs_sorted_entities(system_id)) {
				auto system_name = ecsact::meta::decl_full_name(system_id);
				auto system_cpp_ident =
					"::" + cc_lang_support::cpp_identifier(system_name);

				auto system_sorting_struct_name =
					std::format("system_sorted<{}>", system_cpp_ident);

				ctx.write(
					"reg.emplace_or_replace<",
					system_sorting_struct_name,
					">(entity);"
				);
			}
		}
	}
}

auto ecsact::rt_entt_codegen::core::print_system_marker_remove_fn(
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void {
	for(auto comp : details.all_components) {
		auto comp_name = ecsact::meta::decl_full_name(comp);
		auto comp_cpp_ident = cc_lang_support::cpp_identifier(comp_name);
		auto sorting_structs_covered = std::set<ecsact_system_id>{};

		ctx.write("template<>\n");
		auto printer =
			method_printer{
				ctx,
				"ecsact::entt::detail::remove_system_markers_if_needed<" +
					comp_cpp_ident + ">"
			}
				.parameter("ecsact::entt::registry_t&", "reg")
				.parameter("ecsact::entt::entity_id", "entity")
				.return_type("void");

		ctx.write("//TODO\n");
	}
}

auto ecsact::rt_entt_codegen::core::print_add_sys_beforestorage_fn(
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void {
	using cc_lang_support::cpp_identifier;
	using ecsact::meta::decl_full_name;

	auto already_printed_ = std::set<ecsact_component_id>{};

	for(auto comp_id : details.all_components) {
		auto comp_name = cpp_identifier(decl_full_name(comp_id));

		ctx.write("template<>\n");
		auto printer =
			method_printer{
				ctx,
				std::format(
					"ecsact::entt::detail::add_exec_itr_beforechange_if_needed< {}>",
					comp_name
				)
			}
				.parameter("ecsact::entt::registry_t&", "reg")
				.parameter("ecsact::entt::entity_id", "entity")
				.return_type("void");

		for(auto system_id : details.all_systems) {
			auto notify_settings = ecsact::meta::system_notify_settings(system_id);

			for(auto const [comp_id_to_compare, notify_setting] : notify_settings) {
				if(comp_id == static_cast<ecsact_component_id>(comp_id_to_compare)) {
					if(notify_setting == ECSACT_SYS_NOTIFY_ONCHANGE) {
						if(already_printed_.contains(comp_id)) {
							break;
						}
						already_printed_.insert(comp_id);
						ctx.write(std::format( //
							"reg.emplace<exec_itr_beforechange_storage<{}>>(entity);\n",
							comp_name
						));
						break;
					}
				}
			}
		}
	}
}
