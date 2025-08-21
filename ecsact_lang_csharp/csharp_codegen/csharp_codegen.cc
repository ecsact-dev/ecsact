#include <vector>
#include <cassert>
#include "ecsact/runtime/meta.hh"
#include "ecsact/codegen/plugin.h"
#include "ecsact/codegen/plugin.hh"
#include "ecsact/lang-support/lang-csharp.hh"

static bool has_parent_system(ecsact_system_id id) {
	auto parent_id = ecsact_meta_get_parent_system_id(id);
	return parent_id != (ecsact_system_like_id)-1;
}

static void write_fields(
	ecsact::codegen_plugin_context& ctx,
	ecsact_composite_id             compo_id,
	std::string_view                indentation
) {
	using ecsact::csharp_lang_support::csharp_field_attribute_str;
	using ecsact::csharp_lang_support::csharp_type_str;
	using ecsact::meta::get_field_ids;
	using namespace std::string_literals;

	auto field_ids = get_field_ids(compo_id);
	auto full_name =
		ecsact_meta_decl_full_name(ecsact_id_cast<ecsact_decl_id>(compo_id));

	for(auto field_id : field_ids) {
		auto field_type = ecsact_meta_field_type(compo_id, field_id);
		auto field_name = ecsact_meta_field_name(compo_id, field_id);
		auto attr_str = csharp_field_attribute_str(field_type);
		if(!attr_str.empty()) {
			ctx.write(indentation, attr_str, "\n");
		}

		ctx.write(indentation, "public ");
		switch(field_type.kind) {
			case ECSACT_TYPE_KIND_BUILTIN:
				ctx.write(csharp_type_str(field_type.type.builtin));
				break;
			case ECSACT_TYPE_KIND_ENUM:
				ctx.write(ecsact_meta_enum_name(field_type.type.enum_id));
				break;
		}
		ctx.write(" "s, field_name);

		if(field_type.length > 1) {
			ctx.write("[]");
		}
		ctx.write(";\n");
	}

	ctx.write("\n");
	ctx.write(indentation, "public override bool Equals(object? obj) {\n");
	ctx.write(indentation, "\tif(obj == null) return false;\n");
	ctx.write(indentation, "\tvar other_ = (", full_name, ")obj;\n");
	ctx.write(indentation, "\treturn true");
	for(auto field_id : field_ids) {
		auto field_type = ecsact_meta_field_type(compo_id, field_id);
		auto field_name = ecsact_meta_field_name(compo_id, field_id);

		if(field_type.length > 1) {
			for(int index = 0; field_type.length > index; ++index) {
				ctx.write("\n", indentation, "\t\t");
				ctx.write(
					"&& other_.",
					field_name,
					"[",
					index,
					"].Equals(",
					"this.",
					field_name,
					"[",
					index,
					"]",
					")"
				);
			}
		} else {
			ctx.write("\n", indentation, "\t\t");
			ctx.write("&& other_.", field_name, ".Equals(this.", field_name, ")");
		}
	}
	ctx.write(";\n");
	ctx.write(indentation, "}\n");

	ctx.write("\n");
	ctx.write(indentation, "public override int GetHashCode() {\n");
	ctx.write(indentation, "\tint hashCode_ = 17;\n");
	for(auto field_id : field_ids) {
		auto field_type = ecsact_meta_field_type(compo_id, field_id);
		auto field_name = ecsact_meta_field_name(compo_id, field_id);

		if(field_type.length > 1) {
			for(int index = 0; field_type.length > index; ++index) {
				ctx.write(indentation, "\thashCode_ = hashCode_ * 23 + ");
				ctx.write(field_name, "[", index, "].GetHashCode();\n");
			}
		} else {
			ctx.write(indentation, "\thashCode_ = hashCode_ * 23 + ");
			ctx.write(field_name, ".GetHashCode();\n");
		}
	}
	ctx.write(indentation, "\treturn hashCode_;\n");
	ctx.write(indentation, "}\n");
}

template<typename T>
static void write_id_property(
	ecsact::codegen_plugin_context& ctx,
	T                               id,
	std::string_view                indentation
) {
	ctx.write(
		indentation,
		"public static readonly global::System.Int32 id = ",
		static_cast<int32_t>(ecsact_id_cast<ecsact_decl_id>(id)),
		";\n"
	);
}

static void write_system_struct(
	ecsact::codegen_plugin_context& ctx,
	ecsact_system_id                sys_id,
	std::string                     indentation
) {
	using ecsact::meta::get_child_system_ids;
	using namespace std::string_literals;

	std::string sys_name = ecsact_meta_system_name(sys_id);
	if(!sys_name.empty()) {
		ctx.write(indentation, "public struct ", sys_name);
		ctx.write(" : global::Ecsact.System {\n");
		write_id_property(ctx, sys_id, indentation + "\t");
		for(auto child_system_id : get_child_system_ids(sys_id)) {
			write_system_struct(ctx, child_system_id, indentation + "\t");
		}
		ctx.write(indentation, "}\n"s);
	} else {
		for(auto child_system_id : get_child_system_ids(sys_id)) {
			write_system_struct(ctx, child_system_id, indentation);
		}
	}
}

void ecsact_codegen_plugin(
	ecsact_package_id          package_id,
	ecsact_codegen_write_fn_t  write_fn,
	ecsact_codegen_report_fn_t report_fn
) {
	using ecsact::meta::get_action_ids;
	using ecsact::meta::get_child_system_ids;
	using ecsact::meta::get_component_ids;
	using ecsact::meta::get_enum_ids;
	using ecsact::meta::get_enum_values;
	using ecsact::meta::get_system_ids;
	using ecsact::meta::get_transient_ids;

	ecsact::codegen_plugin_context ctx{package_id, 0, write_fn, report_fn};

	ctx.write("// GENERATED FILE - DO NOT EDIT\n\n");
	ctx.write("#nullable enable\n\n");
	ctx.write("namespace ", ecsact_meta_package_name(package_id), " {\n");

	for(auto enum_id : get_enum_ids(ctx.package_id)) {
		ctx.write("\npublic enum ", ecsact_meta_enum_name(enum_id), " {\n");

		for(auto& enum_value : get_enum_values(enum_id)) {
			ctx.write("\t", enum_value.name, " = ", enum_value.value, ",\n");
		}

		ctx.write("}\n");
	}

	for(auto comp_id : get_component_ids(package_id)) {
		auto compo_id = ecsact_id_cast<ecsact_composite_id>(comp_id);
		auto comp_name = ecsact_meta_component_name(comp_id);
		ctx.write("\npublic struct ", comp_name, " : global::Ecsact.Component {\n");
		write_id_property(ctx, comp_id, "\t");
		write_fields(ctx, compo_id, "\t");
		ctx.write("}\n");
	}

	for(auto trans_id : get_transient_ids(package_id)) {
		auto compo_id = ecsact_id_cast<ecsact_composite_id>(trans_id);
		auto comp_name = ecsact_meta_transient_name(trans_id);
		ctx.write("\npublic struct ", comp_name, " : global::Ecsact.Transient {\n");
		write_id_property(ctx, trans_id, "\t");
		write_fields(ctx, compo_id, "\t");
		ctx.write("}\n");
	}

	for(auto act_id : get_action_ids(package_id)) {
		auto compo_id = ecsact_id_cast<ecsact_composite_id>(act_id);
		auto action_name = ecsact_meta_action_name(act_id);
		ctx.write("\npublic struct ", action_name, " : global::Ecsact.Action {\n");
		write_id_property(ctx, act_id, "\t");
		for(auto child_system_id : get_child_system_ids(act_id)) {
			write_system_struct(ctx, child_system_id, "\t");
		}
		write_fields(ctx, compo_id, "\t");
		ctx.write("}\n");
	}

	for(auto sys_id : get_system_ids(ctx.package_id)) {
		if(has_parent_system(sys_id)) {
			continue;
		}

		write_system_struct(ctx, sys_id, "");
	}

	ctx.write("\n}\n");
}
