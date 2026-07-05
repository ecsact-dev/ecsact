#include "core.hh"

#include "ecsact/lang-support/lang-cc.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"
#include "rt_entt_codegen/shared/util.hh"

auto ecsact::rt_entt_codegen::core::print_copy_components(
	ecsact::codegen_plugin_context&                     ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_details& details
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::meta::decl_full_name;
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "ecsact::entt::copy_components"}
			.parameter("const ::entt::registry&", "src")
			.parameter("::entt::registry&", "dst")
			.return_type("void");

	auto copy_tag_or_component = [&](
		std::string_view cpp_type_name,
		bool             is_tag
	) {
		if(is_tag) {
			block(
				ctx,
				std::format("for(auto entity : src.view<{}>())", cpp_type_name),
				[&] { ctx.writef("dst.emplace<{}>(entity);\n", cpp_type_name); }
			);
		} else {
			block(
				ctx,
				std::format(
					"for(auto&& [entity, comp] : src.view<{}>().each())",
					cpp_type_name
				),
				[&] { ctx.writef("dst.emplace<{}>(entity, comp);\n", cpp_type_name); }
			);
		}
	};

	for(auto comp_id : details.all_components) {
		const auto cpp_comp_name = cpp_identifier(decl_full_name(comp_id));
		const auto is_tag = ecsact::meta::get_field_ids(comp_id).empty();

		copy_tag_or_component(cpp_comp_name, is_tag);

		copy_tag_or_component(
			std::format("::ecsact::entt::detail::pending_add<::{}>", cpp_comp_name),
			is_tag
		);
		copy_tag_or_component(
			std::format("::ecsact::entt::detail::pending_remove<::{}>", cpp_comp_name),
			true
		);
		copy_tag_or_component(
			std::format("::ecsact::entt::detail::run_on_stream<::{}>", cpp_comp_name),
			true
		);
		copy_tag_or_component(
			std::format(
				"::ecsact::entt::detail::beforeremove_storage<::{}>",
				cpp_comp_name
			),
			is_tag
		);

		if(!is_tag) {
			copy_tag_or_component(
				std::format(
					"::ecsact::entt::detail::exec_beforechange_storage<::{}>",
					cpp_comp_name
				),
				false
			);
			copy_tag_or_component(
				std::format(
					"::ecsact::entt::detail::exec_itr_beforechange_storage<::{}>",
					cpp_comp_name
				),
				false
			);
		}

		copy_tag_or_component(
			std::format("::ecsact::entt::component_added<::{}>", cpp_comp_name),
			true
		);
		copy_tag_or_component(
			std::format("::ecsact::entt::component_removed<::{}>", cpp_comp_name),
			true
		);
	}

	for(auto sys_id : details.all_systems) {
		const auto system_name = cpp_identifier(decl_full_name(sys_id));

		copy_tag_or_component(
			std::format(
				"::ecsact::entt::detail::pending_lazy_execution<::{}>",
				system_name
			),
			true
		);
		copy_tag_or_component(
			std::format("::ecsact::entt::detail::system_sorted<::{}>", system_name),
			false
		);
		copy_tag_or_component(
			std::format("::ecsact::entt::detail::run_system<::{}>", system_name),
			true
		);
	}

	copy_tag_or_component("::ecsact::entt::detail::created_entity", false);
	copy_tag_or_component("::ecsact::entt::detail::destroyed_entity", true);
}
