#include "core.hh"
#include "algorithm"

#include "ecsact/lang-support/lang-cc.hh"
#include "rt_entt_codegen/core/core.hh"
#include "rt_entt_codegen/shared/util.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"
#include "rt_entt_codegen/shared/comps_with_caps.hh"

auto ecsact::rt_entt_codegen::core::print_cleanup_ecsact_component_events( //
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::meta::decl_full_name;
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "cleanup_component_events"}
			.parameter("ecsact_registry_id", "registry_id")
			.return_type("void");

	for(auto component_id : details.all_components) {
		auto type_name = cpp_identifier(decl_full_name(component_id));
		ctx.writef(
			"{}{}{}{}",
			"ecsact::entt::wrapper::core::clear_component",
			"<::",
			type_name,
			">(registry_id);\n"
		);
	}
}
