#include <ranges>
#include <format>
#include "core/core.hh"
#include "ecsact/runtime/meta.hh"
#include "ecsact/runtime/meta.h"
#include "ecsact/codegen/plugin.h"
#include "ecsact/codegen/plugin.hh"
#include "ecsact/lang-support/lang-cc.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"

#include "rt_entt_codegen/core/core.hh"
#include "rt_entt_codegen/shared/ecsact_entt_details.hh"
#include "shared/util.hh"

using ecsact::cpp_codegen_plugin_util::block;

constexpr auto GENERATED_FILE_DISCLAIMER = R"(
// GENERATED FILE - DO NOT EDIT
)";

constexpr auto MAIN_PACKAGE_ONLY_DISCLAIMER = R"(
// Purposely empty. ecsact_rt_entt_codegen is only for the 'main' package
)";

/***
 * @returns true if detected unsupported features
 */
static auto check_unsupported_features( //
	ecsact::codegen_plugin_context&                     ctx,
	const ecsact::rt_entt_codegen::ecsact_entt_details& details
) -> bool {
	auto found_assoc_feature = false;
	for(auto comp_id : details.all_components) {
		for(auto field_id : ecsact::meta::get_field_ids(comp_id)) {
			auto field_type = ecsact::meta::get_field_type(comp_id, field_id);
			if(field_type.kind == ECSACT_TYPE_KIND_BUILTIN &&
				 field_type.type.builtin == ECSACT_ENTITY_TYPE) {
				auto comp_name = ecsact::meta::decl_full_name(comp_id);
				auto field_name = ecsact::meta::field_name(comp_id, field_id);
				ctx.error("Assoc field found {}.{}", comp_name, field_name);
				found_assoc_feature = true;
			} else if(field_type.kind == ECSACT_TYPE_KIND_FIELD_INDEX) {
				auto comp_name = ecsact::meta::decl_full_name(comp_id);
				auto field_name = ecsact::meta::field_name(comp_id, field_id);
				ctx.error("Assoc field found {}.{}", comp_name, field_name);
				found_assoc_feature = true;
			}
		}
	}

	for(auto sys_id : details.all_systems) {
		auto assoc_ids = ecsact::meta::system_assoc_ids(sys_id);
		if(!assoc_ids.empty()) {
			found_assoc_feature = true;
			auto system_name = ecsact::meta::decl_full_name(sys_id);
			ctx.error("Assoc system found {}", system_name);
			found_assoc_feature = true;
		}
	}

	for(auto act_id : details.all_actions) {
		auto assoc_ids = ecsact::meta::system_assoc_ids(act_id);
		if(!assoc_ids.empty()) {
			found_assoc_feature = true;
			auto system_name = ecsact::meta::decl_full_name(act_id);
			ctx.error("Assoc action found {}", system_name);
			found_assoc_feature = true;
		}

		for(auto field_id : ecsact::meta::get_field_ids(act_id)) {
			auto field_type = ecsact::meta::get_field_type(act_id, field_id);
			if(field_type.kind == ECSACT_TYPE_KIND_BUILTIN &&
				 field_type.type.builtin == ECSACT_ENTITY_TYPE) {
				auto act_name = ecsact::meta::decl_full_name(act_id);
				auto field_name = ecsact::meta::field_name(act_id, field_id);
				ctx.error("Assoc field found {}.{}", act_name, field_name);
				found_assoc_feature = true;
			} else if(field_type.kind == ECSACT_TYPE_KIND_FIELD_INDEX) {
				auto act_name = ecsact::meta::decl_full_name(act_id);
				auto field_name = ecsact::meta::field_name(act_id, field_id);
				ctx.error("Assoc field found {}.{}", act_name, field_name);
				found_assoc_feature = true;
			}
		}
	}

	if(found_assoc_feature) {
		ctx.fatal(
			"Association currently unsupported "
			"https://github.com/ecsact-dev/ecsact_rt_entt/issues/138"
		);
		return true;
	}

	return false;
}

void ecsact_codegen_plugin(
	ecsact_package_id          package_id,
	ecsact_codegen_write_fn_t  write_fn,
	ecsact_codegen_report_fn_t report_fn
) {
	using ecsact::cc_lang_support::c_identifier;
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::meta::decl_full_name;
	using ecsact::meta::get_all_system_like_ids;
	using ecsact::rt_entt_codegen::ecsact_entt_details;
	using ecsact::rt_entt_codegen::util::global_initializer_printer;
	using ecsact::rt_entt_codegen::util::inc_header;
	using ecsact::rt_entt_codegen::util::inc_package_header;
	using ecsact::rt_entt_codegen::util::init_global;

	ecsact::codegen_plugin_context ctx{package_id, 0, write_fn, report_fn};

	ctx.write(GENERATED_FILE_DISCLAIMER);

	if(ecsact_meta_main_package() != package_id) {
		ctx.write(MAIN_PACKAGE_ONLY_DISCLAIMER);
		return;
	}

	auto details = ecsact_entt_details::from_package(package_id);

	if(check_unsupported_features(ctx, details)) {
		return;
	}

	inc_header(ctx, "ecsact/entt/entity.hh");
	inc_header(ctx, "ecsact/entt/event_markers.hh");
	inc_header(ctx, "ecsact/entt/execution.hh");
	inc_header(ctx, "ecsact/entt/registry_util.hh");
	inc_header(ctx, "ecsact/entt/detail/globals.hh");
	inc_header(ctx, "ecsact/entt/detail/apply_pending.hh");
	inc_header(ctx, "ecsact/entt/detail/registry.hh");
	inc_header(ctx, "ecsact/entt/detail/bytes.hh");
	inc_header(ctx, "ecsact/entt/detail/apply_component_stream_data.hh");
	inc_header(ctx, "ecsact/entt/detail/hash.hh");
	inc_header(ctx, "ecsact/entt/wrapper/core.hh");
	inc_header(ctx, "ecsact/entt/wrapper/dynamic.hh");
	inc_header(ctx, "ecsact/entt/error_check.hh");
	inc_header(ctx, "ecsact/entt/detail/id_map.hh");
	inc_header(ctx, "xxhash.h");
	ctx.write("#include <execution>\n");

	ctx.write("\n");
	inc_package_header(ctx, package_id);
	for(auto dep : ecsact::meta::get_dependencies(package_id)) {
		assert(package_id != dep);
		inc_package_header(ctx, dep);
	}
	ctx.write("\n");

	ctx.write("// test1234\n");
	ctx.write(
		"using exec_entry_t = std::pair<ecsact::entt::execute_fn_t, const "
		"ecsact::entt::actions_map&>;\n\n"
	);

	init_global(ctx, "registries");
	init_global(ctx, "last_registry_id");
	init_global(ctx, "system_impls");

	init_global(ctx, "all_component_ids", [&] {
		if(details.all_components.empty()) {
			return;
		}

		ctx.write("result.reserve(", details.all_components.size(), ");\n");

		for(auto comp_id : details.all_components) {
			auto cpp_comp_name = cpp_identifier(decl_full_name(comp_id));
			ctx.write("result.insert(", cpp_comp_name, "::id);\n");
		}
	});

	init_global(ctx, "add_component_fns", [&] {
		if(details.all_components.empty()) {
			return;
		}

		ctx.write("result.reserve(", details.all_components.size(), ");\n");

		for(auto comp_id : details.all_components) {
			auto cpp_comp_name = cpp_identifier(decl_full_name(comp_id));
			ctx.write(
				"result.insert({::",
				cpp_comp_name,
				"::id, ",
				"&ecsact::entt::wrapper::core::add_component<::",
				cpp_comp_name,
				">});\n"
			);
		}
	});

	init_global(ctx, "get_component_fns", [&] {
		if(details.all_components.empty()) {
			return;
		}

		ctx.write(
			"result.reserve(",
			std::ranges::distance(details.all_components),
			");\n"
		);

		for(auto comp_id : details.all_components) {
			auto cpp_comp_name = cpp_identifier(decl_full_name(comp_id));
			ctx.write(
				"result.insert({::",
				cpp_comp_name,
				"::id, ",
				"&ecsact::entt::wrapper::core::get_component<::",
				cpp_comp_name,
				">});\n"
			);
		}
	});

	init_global(ctx, "update_component_fns", [&] {
		if(details.all_components.empty()) {
			return;
		}

		auto non_tag_component_ids =
			details.all_components |
			std::views::filter([&](ecsact_component_id comp_id) -> bool {
				return !ecsact::meta::get_field_ids(comp_id).empty();
			});

		ctx.write(
			"result.reserve(",
			std::ranges::distance(non_tag_component_ids),
			");\n"
		);

		for(auto comp_id : non_tag_component_ids) {
			auto cpp_comp_name = cpp_identifier(decl_full_name(comp_id));
			ctx.write(
				"result.insert({::",
				cpp_comp_name,
				"::id, ",
				"&ecsact::entt::wrapper::core::update_component<::",
				cpp_comp_name,
				">});\n"
			);
		}
	});

	init_global(ctx, "remove_component_fns", [&] {
		if(details.all_components.empty()) {
			return;
		}

		ctx.write("result.reserve(", details.all_components.size(), ");\n");

		for(auto comp_id : details.all_components) {
			auto cpp_comp_name = cpp_identifier(decl_full_name(comp_id));
			ctx.write(
				"result.insert({::",
				cpp_comp_name,
				"::id, ",
				"&ecsact::entt::wrapper::core::remove_component<::",
				cpp_comp_name,
				">});\n"
			);
		}
	});

	init_global(ctx, "has_component_fns", [&] {
		if(details.all_components.empty()) {
			return;
		}

		ctx.write("result.reserve(", details.all_components.size(), ");\n");

		for(auto comp_id : details.all_components) {
			auto cpp_comp_name = cpp_identifier(decl_full_name(comp_id));
			ctx.write(
				"result.insert({::",
				cpp_comp_name,
				"::id, ",
				"&ecsact::entt::wrapper::core::has_component<::",
				cpp_comp_name,
				">});\n"
			);
		}
	});

	init_global(ctx, "ecsact_stream_fns", [&] {
		if(details.all_components.empty()) {
			return;
		}

		std::unordered_set<ecsact_component_id> stream_components;

		for(auto comp_id : details.all_components) {
			auto comp_type = ecsact_meta_component_type(
				ecsact_id_cast<ecsact_component_like_id>(comp_id)
			);

			if(comp_type == ECSACT_COMPONENT_TYPE_STREAM ||
				 comp_type == ECSACT_COMPONENT_TYPE_LAZY_STREAM) {
				stream_components.insert(comp_id);
			}
		}

		ctx.write("result.reserve(", stream_components.size(), ");\n");

		for(auto comp_id : stream_components) {
			auto cpp_comp_name = cpp_identifier(decl_full_name(comp_id));
			ctx.write(
				"result.insert({::",
				cpp_comp_name,
				"::id, ",
				"&ecsact::entt::wrapper::core::ecsact_stream<::",
				cpp_comp_name,
				">});\n"
			);
		}
	});

	ctx.write("\n");

	for(auto comp_id : details.all_components) {
		auto cpp_comp_name = cpp_identifier(decl_full_name(comp_id));
		block(
			ctx,
			std::format(
				"template<> struct entt::component_traits<{}>",
				cpp_comp_name
			),
			[&] {
				ctx.write("using type = ::", cpp_comp_name, ";\n");
				ctx.write("static constexpr bool in_place_delete = true;\n");
				ctx.write("static constexpr std::size_t page_size = ENTT_PACKED_PAGE;\n"
				);
			}
		);
		ctx.write(";\n");
	}

	{ // Print core Ecsact API methods
		using namespace ecsact::rt_entt_codegen;

		core::print_entity_match_fn(ctx, details);
		core::print_system_marker_add_fn(ctx, details);
		core::print_system_marker_remove_fn(ctx, details);
		core::print_add_sys_beforestorage_fn(ctx, details);
		core::print_entity_sorting_components(ctx, details);
		core::print_parallel_system_execute(ctx, details);
		core::print_check_error_template_specializations(ctx, details);
		core::print_execute_system_like_template_specializations(ctx, details);
		core::print_init_registry_storage(ctx, details);
		core::print_copy_components(ctx, details);
		core::print_hash_registry(ctx, details);
		core::print_apply_streaming_data(ctx, details);
		core::print_trigger_ecsact_events_minimal(ctx, details);
		core::print_trigger_ecsact_events_all(ctx, details);
		core::print_cleanup_ecsact_component_events(ctx, details);
		core::print_execution_options(ctx, details);
		core::print_cleanup_system_notifies(ctx, details);
		core::print_update_all_beforechange_storage(ctx, details);
		core::print_execute_systems(ctx, details);
	}
}
