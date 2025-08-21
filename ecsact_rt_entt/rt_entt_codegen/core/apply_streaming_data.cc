
#include "core.hh"

#include "rt_entt_codegen/shared/parallel.hh"
#include "ecsact/lang-support/lang-cc.hh"
#include "rt_entt_codegen/shared/util.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"
#include "ecsact/runtime/meta.h"

auto ecsact::rt_entt_codegen::core::print_apply_streaming_data(
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::meta::decl_full_name;

	block(
		ctx,
		"auto apply_streaming_data(::entt::registry& main_reg) -> void",
		[&] {
			ctx.write(
				"auto stream_registries = "
				"ecsact::entt::detail::globals::stream_registries.get_stream_"
				"registries();\n"
			);

			block(ctx, "for(auto& stream_reg: stream_registries)", [&] {
				for(auto component_id : details.all_components) {
					auto comp_type = ecsact_meta_component_type(
						ecsact_id_cast<ecsact_component_like_id>(component_id)
					);

					if(comp_type == ECSACT_COMPONENT_TYPE_LAZY_STREAM ||
						 comp_type == ECSACT_COMPONENT_TYPE_STREAM) {
						auto comp_name = cpp_identifier(decl_full_name(component_id));

						ctx.write("ecsact::entt::detail::apply_component_stream_data<");
						ctx.write(comp_name);
						ctx.write(">(main_reg, *stream_reg);\n");
					}
				}
			});
		}
	);
}
