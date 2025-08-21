#include <vector>
#include <string>
#include <cassert>
#include "ecsact/runtime/meta.hh"
#include "ecsact/codegen/plugin.h"
#include "ecsact/codegen/plugin.hh"
#include "ecsact/lang-support/lang-cc.hh"

constexpr auto GENERATED_FILE_DISCLAIMER = R"(// GENERATED FILE - DO NOT EDIT
)";

template<typename T>
static void write_constexpr_id(
	ecsact::codegen_plugin_context& ctx,
	const char*                     id_type_name,
	T                               id,
	std::string_view                indentation
) {
	ctx.writef(
		"{}static constexpr auto id = static_cast<{}>({});\n",
		indentation,
		id_type_name,
		static_cast<int32_t>(id)
	);
}

template<typename T>
static std::vector<ecsact_field_id> get_field_ids(T id) {
	ecsact_composite_id compo_id;
	if constexpr(std::is_same_v<ecsact_composite_id, T>) {
		compo_id = id;
	} else {
		compo_id = ecsact_id_cast<ecsact_composite_id>(id);
	}
	std::vector<ecsact_field_id> field_ids;
	field_ids.resize(ecsact_meta_count_fields(compo_id));
	ecsact_meta_get_field_ids(
		compo_id,
		static_cast<int32_t>(field_ids.size()),
		field_ids.data(),
		nullptr
	);

	return field_ids;
}

static bool has_parent_system(ecsact_system_id id) {
	auto parent_id = ecsact_meta_get_parent_system_id(id);
	return parent_id != (ecsact_system_like_id)-1;
}

static auto cpp_field_type_name(ecsact_field_type field_type) -> std::string {
	using ecsact::cc_lang_support::cpp_type_str;
	switch(field_type.kind) {
		case ECSACT_TYPE_KIND_BUILTIN:
			return cpp_type_str(field_type.type.builtin);
		case ECSACT_TYPE_KIND_ENUM:
			return ecsact_meta_enum_name(field_type.type.enum_id);
		case ECSACT_TYPE_KIND_FIELD_INDEX: {
			auto field_index_field_type = ecsact_meta_field_type(
				field_type.type.field_index.composite_id,
				field_type.type.field_index.field_id
			);
			return cpp_field_type_name(field_index_field_type);
		}
	}
}

static void write_fields(
	ecsact::codegen_plugin_context& ctx,
	ecsact_composite_id             compo_id,
	std::string_view                indentation
) {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::cc_lang_support::cpp_type_str;
	using namespace std::string_literals;

	auto full_name =
		ecsact_meta_decl_full_name(ecsact_id_cast<ecsact_decl_id>(compo_id));

	for(auto field_id : get_field_ids(compo_id)) {
		auto field_type = ecsact_meta_field_type(compo_id, field_id);
		auto field_name = ecsact_meta_field_name(compo_id, field_id);

		ctx.writef(
			"{}{} {}",
			indentation,
			cpp_field_type_name(field_type),
			field_name
		);

		if(field_type.length > 1) {
			ctx.writef("[{}]", field_type.length);
		}
		ctx.writef(";\n");
	}

	ctx.writef(
		"{}auto operator<=>(const {}&) const = default;\n",
		indentation,
		cpp_identifier(full_name)
	);
}

static void write_system_impl_decl(
	ecsact::codegen_plugin_context& ctx,
	std::string_view                indentation
) {
	ctx.writef("{}struct context;\n", indentation);
	ctx.writef("{}static void impl(context&);\n", indentation);
}

static void write_system_struct(
	ecsact::codegen_plugin_context& ctx,
	ecsact_system_id                sys_id,
	std::string                     indentation
) {
	using namespace std::string_literals;
	using ecsact::cc_lang_support::anonymous_system_name;
	using ecsact::meta::get_child_system_ids;

	std::string sys_name = ecsact_meta_system_name(sys_id);
	if(!sys_name.empty()) {
		ctx.writef("{}struct {} {{\n", indentation, sys_name);
		write_constexpr_id(ctx, "ecsact_system_id", sys_id, indentation + "\t");
		for(auto child_system_id : get_child_system_ids(sys_id)) {
			write_system_struct(ctx, child_system_id, indentation + "\t");
		}
		write_system_impl_decl(ctx, indentation + "\t");
		ctx.writef("{}}};\n", indentation);
	} else {
		ctx.writef("{}struct {} {{\n", indentation, anonymous_system_name(sys_id));
		write_constexpr_id(ctx, "ecsact_system_id", sys_id, indentation + "\t");
		ctx.writef("{}\tstruct context;\n", indentation);
		ctx.writef("{}}};\n", indentation);

		for(auto child_system_id : get_child_system_ids(sys_id)) {
			write_system_struct(ctx, child_system_id, indentation);
		}
	}
}

template<typename CompositeID>
static auto has_assoc_fields(CompositeID compo_id) -> bool {
	for(auto field_id : ecsact::meta::get_field_ids(compo_id)) {
		auto field_type = ecsact::meta::get_field_type(compo_id, field_id);
		if(field_type.kind == ECSACT_TYPE_KIND_BUILTIN) {
			if(field_type.type.builtin == ECSACT_ENTITY_TYPE) {
				return true;
			}
		}

		if(field_type.kind == ECSACT_TYPE_KIND_FIELD_INDEX) {
			return true;
		}
	}

	return false;
}

void ecsact_codegen_plugin(
	ecsact_package_id          package_id,
	ecsact_codegen_write_fn_t  write_fn,
	ecsact_codegen_report_fn_t report_fn
) {
	using ecsact::cc_lang_support::cpp_identifier;
	using namespace std::string_literals;
	using ecsact::meta::get_action_ids;
	using ecsact::meta::get_child_system_ids;
	using ecsact::meta::get_component_ids;
	using ecsact::meta::get_enum_ids;
	using ecsact::meta::get_enum_values;
	using ecsact::meta::get_system_ids;
	using ecsact::meta::get_transient_ids;

	ecsact::codegen_plugin_context ctx{package_id, 0, write_fn, report_fn};

	ctx.writef(GENERATED_FILE_DISCLAIMER);
	ctx.writef("#pragma once\n\n");

	ctx.writef("#include <cstdint>\n");
	ctx.writef("#include <compare>\n");
	ctx.writef("#include \"ecsact/runtime/common.h\"\n");
	ctx.writef("\n");

	const auto namespace_str =
		cpp_identifier(ecsact_meta_package_name(ctx.package_id));

	ctx.writef("namespace {} {{\n\n", namespace_str);

	for(auto enum_id : get_enum_ids(ctx.package_id)) {
		ctx.writef("enum class {} {{", ecsact_meta_enum_name(enum_id));
		++ctx.indentation;
		ctx.writef("\n");

		for(auto& enum_value : get_enum_values(enum_id)) {
			ctx.writef("{} = {},\n", enum_value.name, enum_value.value);
		}
		ctx.writef("}};");
		--ctx.indentation;
	}

	for(auto comp_id : get_component_ids(ctx.package_id)) {
		auto compo_id = ecsact_id_cast<ecsact_composite_id>(comp_id);
		ctx.writef("struct {} {{\n", ecsact_meta_component_name(comp_id));
		ctx.writef("\tstatic constexpr bool transient = false;\n");
		ctx.writef(
			"\tstatic constexpr bool has_assoc_fields = {};\n",
			has_assoc_fields(comp_id) ? "true" : "false"
		);
		write_constexpr_id(ctx, "ecsact_component_id", comp_id, "\t");
		write_fields(ctx, compo_id, "\t"s);
		ctx.writef("}};\n");
	}

	for(auto comp_id : get_transient_ids(ctx.package_id)) {
		auto compo_id = ecsact_id_cast<ecsact_composite_id>(comp_id);
		ctx.writef("struct {} {{\n", ecsact_meta_transient_name(comp_id));
		ctx.writef("\tstatic constexpr bool transient = true;\n");
		ctx.writef(
			"\tstatic constexpr bool has_assoc_fields = {};\n",
			has_assoc_fields(comp_id) ? "true" : "false"
		);
		write_constexpr_id(ctx, "ecsact_transient_id", comp_id, "\t");
		write_fields(ctx, compo_id, "\t"s);
		ctx.writef("}};\n");
	}

	for(auto action_id : get_action_ids(ctx.package_id)) {
		auto compo_id = ecsact_id_cast<ecsact_composite_id>(action_id);
		ctx.writef("struct {} {{\n", ecsact_meta_action_name(action_id));
		ctx.writef(
			"\tstatic constexpr bool has_assoc_fields = {};\n",
			has_assoc_fields(action_id) ? "true" : "false"
		);
		write_constexpr_id(ctx, "ecsact_action_id", compo_id, "\t");
		for(auto child_system_id : get_child_system_ids(action_id)) {
			write_system_struct(ctx, child_system_id, "\t");
		}
		write_system_impl_decl(ctx, "\t");
		write_fields(ctx, compo_id, "\t");
		ctx.writef("}};\n");
	}

	for(auto sys_id : get_system_ids(ctx.package_id)) {
		if(has_parent_system(sys_id)) {
			continue;
		}

		write_system_struct(ctx, sys_id, "");
	}

	ctx.writef("\n}}// namespace {}\n", namespace_str);
}
