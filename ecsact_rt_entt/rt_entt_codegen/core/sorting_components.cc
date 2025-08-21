#include "rt_entt_codegen/core/core.hh"

#include <format>
#include "ecsact/runtime/common.h"
#include "ecsact/runtime/meta.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"
#include "rt_entt_codegen/shared/util.hh"
#include "rt_entt_codegen/shared/sorting.hh"

using ecsact::cpp_codegen_plugin_util::method_printer;
using ecsact::rt_entt_codegen::ecsact_entt_details;
using ecsact::rt_entt_codegen::system_needs_sorted_entities;
using ecsact::rt_entt_codegen::util::decl_cpp_ident;

constexpr auto RECALC_COMPONENTS_HASH_DECL = R"cpp(
template<typename S>
static auto _recalc_sorting_hash(ecsact::entt::registry_t&) -> void;
)cpp";

static auto print_system_entity_sorting_component_struct(
	ecsact::codegen_plugin_context&                     ctx,
	ecsact_system_like_id                               sys_like_id,
	ecsact::rt_entt_codegen::ecsact_entt_system_details sys_details
) -> void {
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::rt_entt_codegen::util::has_no_fields;
	using ecsact::rt_entt_codegen::util::make_view;

	auto system_cpp_name = decl_cpp_ident(sys_like_id);
	auto caps_map = ecsact::meta::system_capabilities(sys_like_id);

	auto system_sorting_struct_name =
		std::format("::ecsact::entt::detail::system_sorted<{}>", system_cpp_name);

	ctx.write("template<>\n");
	auto printer =
		method_printer{ctx, "_recalc_sorting_hash<" + system_cpp_name + ">"}
			.parameter("ecsact::entt::registry_t&", "reg")
			.return_type("void");

	auto gen_comp_var_name = [](auto comp_id) -> std::string {
		return std::format("c{}", static_cast<int>(comp_id));
	};

	make_view(
		ctx,
		"view",
		"reg",
		sys_details,
		std::vector{
			system_sorting_struct_name,
		}
	);

	block(ctx, "for(auto entity : view)", [&] {
		ctx.write(
			"auto& sorted = view.get<",
			system_sorting_struct_name,
			">(entity);\n"
		);

		for(auto&& [comp_id, caps] : caps_map) {
			if(has_no_fields(comp_id)) {
				continue;
			}

			auto comp_cpp_name = decl_cpp_ident(comp_id);
			auto comp_var = gen_comp_var_name(comp_id);

			ctx.write(
				"const auto& ",
				comp_var,
				" = ",
				"view.get<",
				comp_cpp_name,
				">(entity);\n"
			);
		}

		ctx.write("auto bytes = ::ecsact::entt::detail::bytes_copy(");
		ctx.indentation += 1;

		auto prefix = std::string{"\n"};

		for(auto&& [comp_id, caps] : caps_map) {
			auto comp_cpp_name = decl_cpp_ident(comp_id);
			auto comp_var = gen_comp_var_name(comp_id);

			for(auto field_id : ecsact::meta::get_field_ids(comp_id)) {
				auto field_name = std::string{ecsact_meta_field_name(
					ecsact_id_cast<ecsact_composite_id>(comp_id),
					field_id
				)};

				ctx.write(prefix, comp_var, ".", field_name);
				prefix = ",\n";
			}
		}

		ctx.indentation -= 1;
		ctx.write("\n);\n");

		ctx.write(
			"sorted.hash = ::ecsact::entt::detail::bytes_hash(bytes.data(), "
			"bytes.size());"
		);
	});
}

auto ecsact::rt_entt_codegen::core::print_entity_sorting_components(
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void {
	ctx.write(RECALC_COMPONENTS_HASH_DECL);

	for(auto sys_id : details.all_systems) {
		auto sys_like_id = ecsact_id_cast<ecsact_system_like_id>(sys_id);
		auto sys_details =
			ecsact_entt_system_details::from_system_like(sys_like_id);

		if(system_needs_sorted_entities(sys_id)) {
			auto sys_like_id = ecsact_id_cast<ecsact_system_like_id>(sys_id);
			auto sys_details =
				ecsact_entt_system_details::from_system_like(sys_like_id);
			print_system_entity_sorting_component_struct(
				ctx,
				sys_like_id,
				sys_details
			);
		}
	}
}
