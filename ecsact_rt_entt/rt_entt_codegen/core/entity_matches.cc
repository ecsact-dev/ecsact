#include "rt_entt_codegen/core/core.hh"

#include <ranges>
#include <format>
#include "ecsact/runtime/meta.hh"
#include "ecsact/lang-support/lang-cc.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"
#include "rt_entt_codegen/shared/util.hh"

using ecsact::cpp_codegen_plugin_util::block;
using ecsact::cpp_codegen_plugin_util::method_printer;

auto ecsact::rt_entt_codegen::core::print_entity_match_fn(
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void {
	using ecsact::rt_entt_codegen::util::decl_cpp_ident;
	using std::views::transform;

	for(auto sys_id : details.all_systems) {
		auto sys_details = ecsact_entt_system_details::from_system_like(
			ecsact_id_cast<ecsact_system_like_id>(sys_id)
		);
		auto system_name = ecsact::meta::decl_full_name(sys_id);
		auto system_cpp_ident = cc_lang_support::cpp_identifier(system_name);
		auto method_name =
			"ecsact::entt::entity_matches_system<" + system_cpp_ident + ">";

		ctx.write("template<>\n");
		auto printer = //
			method_printer{ctx, method_name}
				.parameter("ecsact::entt::registry_t&", "reg")
				.parameter("ecsact::entt::entity_id", "entity")
				.return_type("bool");

		auto get_comps_str = util::comma_delim(
			sys_details.get_comps |
			transform(decl_cpp_ident<ecsact_component_like_id>)
		);

		block(ctx, std::format("if(!reg.all_of<{}>(entity))", get_comps_str), [&] {
			ctx.write("return false;");
		});

		if(!sys_details.exclude_comps.empty()) {
			auto exclude_comps_str = util::comma_delim(
				sys_details.exclude_comps |
				transform(decl_cpp_ident<ecsact_component_like_id>)
			);

			block(
				ctx,
				std::format("if(reg.any_of<{}>(entity))", exclude_comps_str),
				[&] { ctx.write("return false;"); }
			);
		}

		ctx.write("return true;\n");
	}
}
