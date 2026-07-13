#include "core.hh"

#include "ecsact/lang-support/lang-cc.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"
#include "rt_entt_codegen/shared/util.hh"

auto ecsact::rt_entt_codegen::core::print_hash_registry(
	ecsact::codegen_plugin_context&                     ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_details& details
) -> void {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cpp_codegen_plugin_util::block;
	using ecsact::meta::decl_full_name;
	using ecsact::rt_entt_codegen::util::method_printer;

	auto printer = //
		method_printer{ctx, "ecsact::entt::hash_registry"}
			.parameter("const ::entt::registry&", "reg")
			.parameter(
				"std::function<::entt::entity(::entt::entity)>",
				"entity_translator"
			)
			.return_type("std::uint64_t");

	ctx.writef("auto* state = XXH3_createState();\n");
	ctx.writef("if(!state) {{ return 0; }}\n");
	ctx.writef("XXH3_64bits_reset(state);\n");
	ctx.writef("\n");

	// Gather and sort all entities to ensure order-deterministic hashing
	ctx.writef("auto entities = std::vector<::entt::entity>{{}};\n");
	ctx.writef("entities.reserve(reg.template storage<::entt::entity>() ? reg.template storage<::entt::entity>()->size() : 0);\n");
	block(
		ctx,
		"if(auto* storage = reg.template storage<::entt::entity>())",
		[&] {
			block(
				ctx,
				"for(auto&& [entity] : storage->each())",
				[&] {
					ctx.writef("entities.push_back(entity);\n");
				}
			);
		}
	);
	ctx.writef("std::sort(entities.begin(), entities.end(), [&](auto a, auto b) {{\n");
	ctx.writef("\t::entt::entity ta = a;\n");
	ctx.writef("\t::entt::entity tb = b;\n");
	ctx.writef("\tif(entity_translator) {{\n");
	ctx.writef("\t\tta = entity_translator(a);\n");
	ctx.writef("\t\ttb = entity_translator(b);\n");
	ctx.writef("\t}}\n");
	ctx.writef("\tassert(ta != static_cast<::entt::entity>(ECSACT_INVALID_ID(entity)));\n");
	ctx.writef("\tassert(tb != static_cast<::entt::entity>(ECSACT_INVALID_ID(entity)));\n");
	ctx.writef("\treturn ta < tb;\n");
	ctx.writef("}});\n");
	ctx.writef("\n");

	block(
		ctx,
		"for(auto entity : entities)",
		[&] {
			ctx.writef("auto translated_entity = entity;\n");
			ctx.writef("if(entity_translator) {{\n");
			ctx.writef("\ttranslated_entity = entity_translator(entity);\n");
			ctx.writef("}}\n");
			ctx.writef("XXH3_64bits_update(state, &translated_entity, sizeof(translated_entity));\n");
			ctx.writef(
				"auto has_states = std::array<bool, {}>{{\n",
				details.all_components.size() + details.all_systems.size()
			);
			for(auto comp_id : details.all_components) {
				const auto cpp_comp_name = cpp_identifier(decl_full_name(comp_id));
				ctx.writef("\treg.all_of<{}>(entity),\n", cpp_comp_name);
			}

			for(auto sys_id : details.all_systems) {
				const auto system_name = cpp_identifier(decl_full_name(sys_id));
				const auto pending_lazy_exec_struct = std::format(
					"::ecsact::entt::detail::pending_lazy_execution<::{}>",
					system_name
				);
				ctx.writef("\treg.all_of<{}>(entity),\n", pending_lazy_exec_struct);
			}

			ctx.writef("\n}};\n");
			ctx.writef(
				"XXH3_64bits_update(state, has_states.data(), sizeof(bool) * "
				"has_states.size());\n"
			);
		}
	);

	ctx.writef("\n");

	for(auto comp_id : details.all_components) {
		const auto cpp_comp_name = cpp_identifier(decl_full_name(comp_id));
		if(ecsact::meta::get_field_ids(comp_id).empty()) {
			block(
				ctx,
				"for(auto entity : entities)",
				[&] {
					ctx.writef("if(reg.all_of<{}>(entity)) {{\n", cpp_comp_name);
					ctx.writef("\tauto translated_entity = entity;\n");
					ctx.writef("\tif(entity_translator) {{\n");
					ctx.writef("\t\ttranslated_entity = entity_translator(entity);\n");
					ctx.writef("\t}}\n");
					// ctx.writef("\tstd::println(\"[HASH DEBUG] Entity {{}} has tag {{}}\", static_cast<int>(translated_entity), \"{0}\");\n", cpp_comp_name);
					ctx.writef("}}\n");
				}
			);
			continue;
		}

		block(
			ctx,
			"for(auto entity : entities)",
			[&] {
				ctx.writef("if(auto* comp = reg.try_get<{}>(entity)) {{\n", cpp_comp_name);
				ctx.writef("\tauto translated_entity = entity;\n");
				ctx.writef("\tif(entity_translator) {{\n");
				ctx.writef("\t\ttranslated_entity = entity_translator(entity);\n");
				ctx.writef("\t}}\n");
				ctx.writef("\tXXH3_64bits_update(state, &translated_entity, sizeof(translated_entity));\n");
				// ctx.writef("\tstd::print(\"[HASH DEBUG] Entity {{}} has {{}} (size {{}}): \", static_cast<int>(translated_entity), \"{0}\", sizeof({0}));\n", cpp_comp_name);
				// ctx.writef("\tfor(int j=0; j<sizeof({0}); ++j) {{ std::print(\"{{:02x}} \", reinterpret_cast<const uint8_t*>(comp)[j]); }}\n", cpp_comp_name);
				// ctx.writef("\tstd::println(\"\");\n");
				auto field_ids = ecsact::meta::get_field_ids(comp_id);
				for(auto field_id : field_ids) {
					auto field_name = ecsact::meta::field_name(comp_id, field_id);
					auto field_type = ecsact::meta::get_field_type(comp_id, field_id);
					if(field_type.kind == ECSACT_TYPE_KIND_BUILTIN && field_type.type.builtin == ECSACT_ENTITY_TYPE) {
						ctx.writef("\t\tauto translated_field_{0} = comp->{0};\n", field_name);
						ctx.writef("\t\tif(entity_translator) {{\n");
						ctx.writef("\t\t\ttranslated_field_{0} = static_cast<ecsact_entity_id>(\n", field_name);
						ctx.writef("\t\t\t\tentity_translator(static_cast<::entt::entity>(comp->{0}))\n", field_name);
						ctx.writef("\t\t\t);\n");
						ctx.writef("\t\t}}\n");
						ctx.writef(
							"\t\tXXH3_64bits_update(state, &translated_field_{0}, "
							"sizeof(decltype(translated_field_{0})));\n",
							field_name
						);
					} else {
						ctx.writef(
							"\t\tXXH3_64bits_update(state, &comp->{0}, "
							"sizeof(decltype(comp->{0})));\n",
							field_name
						);
					}
				}
				ctx.writef("}}\n");
			}
		);
		ctx.writef("\n");
	}

	ctx.writef("auto result = XXH3_64bits_digest(state);\n");
	// ctx.writef("std::println(\"[HASH DEBUG] Final registry hash: {{}}\", result);\n");
	ctx.writef("XXH3_freeState(state);\n");
	ctx.writef("return result;\n");
}
