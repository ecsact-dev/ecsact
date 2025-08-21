#include "rt_entt_codegen/core/core.hh"
#include "ecsact/lang-support/lang-cc.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"
#include "rt_entt_codegen/shared/system_util.hh"
#include "rt_entt_codegen/shared/util.hh"

auto ecsact::rt_entt_codegen::core::print_cleanup_system_notifies(
	codegen_plugin_context&    ctx,
	const ecsact_entt_details& details
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::meta::decl_full_name;
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "cleanup_system_notifies"}
			.parameter("ecsact_registry_id", "registry_id")
			.return_type("void");

	for(auto system_id : details.all_systems) {
		if(!system_util::is_notify_system(
				 ecsact_id_cast<ecsact_system_like_id>(system_id)
			 )) {
			continue;
		}

		auto system_name = cpp_identifier(decl_full_name(system_id));

		ctx.write(
			"ecsact::entt::wrapper::core::clear_notify_component<",
			system_name,
			">(registry_id);\n"
		);
	}
}

namespace ecsact::rt_entt_codegen::core::notify {
auto print_system_oninit() -> void {
}
} // namespace ecsact::rt_entt_codegen::core::notify
