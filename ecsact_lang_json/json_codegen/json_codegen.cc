#include <vector>
#include <string>
#include <cassert>
#include <filesystem>
#include "ecsact/runtime/meta.hh"
#include "ecsact/codegen/plugin.h"
#include "ecsact/codegen/plugin.hh"
#include "nlohmann/json.hpp"
#include "magic_enum.hpp"

namespace fs = std::filesystem;

static void json_action_properties(
	ecsact_action_id action_id,
	nlohmann::json&  out_json
);

static void json_system_properties(
	ecsact_system_id sys_id,
	nlohmann::json&  out_json
);

static void json_component_properties(
	ecsact_component_id comp_id,
	nlohmann::json&     out_json
);

static void json_transient_properties(
	ecsact_transient_id trans_id,
	nlohmann::json&     out_json
);

template<typename SystemLikeID>
static void json_common_decl_properties(
	SystemLikeID    id,
	nlohmann::json& out_json
) {
	auto decl_id = ecsact_id_cast<ecsact_decl_id>(id);
	out_json["id"] = static_cast<int32_t>(decl_id);
	out_json["full_name"] = ecsact::meta::decl_full_name(decl_id);
}

template<typename SystemLikeID>
static void json_system_like_properties(
	SystemLikeID    id,
	nlohmann::json& out_json
) {
	auto sys_like_id = ecsact_id_cast<ecsact_system_like_id>(id);
	for(auto& child_id : ecsact::meta::get_child_system_ids(sys_like_id)) {
		auto child_json = "{}"_json;
		json_system_properties(child_id, child_json);
		out_json["child_systems"].emplace_back(std::move(child_json));
	}
}

template<typename CompositeID>
static void json_composite_properties(
	CompositeID     id,
	nlohmann::json& out_json
) {
	auto compo_id = ecsact_id_cast<ecsact_composite_id>(id);
	out_json["fields"] = "[]"_json;

	for(auto& field_id : ecsact::meta::get_field_ids(compo_id)) {
		auto field_json = "{}"_json;
		auto field_type_json = "{}"_json;
		auto field_name = ecsact_meta_field_name(compo_id, field_id);
		auto field_type = ecsact_meta_field_type(compo_id, field_id);

		field_type_json["length"] = field_type.length;
		if(field_type.kind == ECSACT_TYPE_KIND_BUILTIN) {
			switch(field_type.type.builtin) {
				case ECSACT_BOOL:
					field_type_json["type"] = "bool";
					break;
				case ECSACT_I8:
					field_type_json["type"] = "i8";
					break;
				case ECSACT_U8:
					field_type_json["type"] = "u8";
					break;
				case ECSACT_I16:
					field_type_json["type"] = "i16";
					break;
				case ECSACT_U16:
					field_type_json["type"] = "u16";
					break;
				case ECSACT_I32:
					field_type_json["type"] = "i32";
					break;
				case ECSACT_U32:
					field_type_json["type"] = "u32";
					break;
				case ECSACT_F32:
					field_type_json["type"] = "f32";
					break;
				case ECSACT_ENTITY_TYPE:
					field_type_json["type"] = "entity";
					break;
			}
		} else if(field_type.kind == ECSACT_TYPE_KIND_ENUM) {
			field_type_json["type"] = ecsact_meta_enum_name(field_type.type.enum_id);
		}

		field_json["field_name"] = field_name;
		field_json["field_type"] = std::move(field_type_json);

		out_json["fields"].emplace_back(std::move(field_json));
	}
}

static void json_action_properties(
	ecsact_action_id action_id,
	nlohmann::json&  out_json
) {
	out_json["name"] = ecsact::meta::action_name(action_id);
	json_common_decl_properties(action_id, out_json);
	json_system_like_properties(action_id, out_json);
	json_composite_properties(action_id, out_json);
}

static void json_system_properties(
	ecsact_system_id sys_id,
	nlohmann::json&  out_json
) {
	out_json["name"] = ecsact::meta::system_name(sys_id);
	json_common_decl_properties(sys_id, out_json);
	json_system_like_properties(sys_id, out_json);
}

static void json_component_properties(
	ecsact_component_id comp_id,
	nlohmann::json&     out_json
) {
	out_json["name"] = ecsact::meta::component_name(comp_id);
	json_common_decl_properties(comp_id, out_json);
	json_composite_properties(comp_id, out_json);
}

static void json_transient_properties(
	ecsact_transient_id trans_id,
	nlohmann::json&     out_json
) {
	out_json["name"] = ecsact::meta::transient_name(trans_id);
	json_common_decl_properties(trans_id, out_json);
	json_composite_properties(trans_id, out_json);
}

void ecsact_codegen_plugin(
	ecsact_package_id          package_id,
	ecsact_codegen_write_fn_t  write_fn,
	ecsact_codegen_report_fn_t report_fn
) {
	using ecsact::meta::get_action_ids;
	using ecsact::meta::get_component_ids;
	using ecsact::meta::get_dependencies;
	using ecsact::meta::get_system_ids;
	using ecsact::meta::get_transient_ids;
	using ecsact::meta::package_name;

	ecsact::codegen_plugin_context ctx{package_id, 0, write_fn, report_fn};
	ctx.write("{\n");

	ctx.write(
		"\t\"source_file_path\": \"",
		fs::path{ecsact_meta_package_file_path(ctx.package_id)}.generic_string(),
		"\",\n"
	);

	ctx.write("\t\"name\": \"", package_name(ctx.package_id), "\",\n");
	ctx.write("\t\"imports\": [");
	ctx.write_each(
		", ",
		get_dependencies(ctx.package_id),
		[&](ecsact_package_id dep_id) {
			ctx.write("\"", package_name(dep_id), "\"");
		}
	);
	ctx.write("],\n");

	{
		auto actions = "[]"_json;
		for(auto& action_id : get_action_ids(ctx.package_id)) {
			auto action = "{}"_json;
			json_action_properties(action_id, action);
			actions.emplace_back(std::move(action));
		}

		ctx.write("\t\"actions\": ", actions.dump(), ",\n");
	}

	{
		auto systems = "[]"_json;
		for(auto& system_id : get_system_ids(ctx.package_id)) {
			auto system = "{}"_json;
			json_system_properties(system_id, system);
			systems.emplace_back(std::move(system));
		}

		ctx.write("\t\"systems\": ", systems.dump(), ",\n");
	}

	{
		auto components = "[]"_json;
		for(auto& component_id : get_component_ids(ctx.package_id)) {
			auto component = "{}"_json;
			json_component_properties(component_id, component);
			components.emplace_back(std::move(component));
		}

		ctx.write("\t\"components\": ", components.dump(), ",\n");
	}

	{
		auto transients = "[]"_json;
		for(auto& transient_id : get_transient_ids(ctx.package_id)) {
			auto transient = "{}"_json;
			json_transient_properties(transient_id, transient);
			transients.emplace_back(std::move(transient));
		}

		ctx.write("\t\"transients\": ", transients.dump(), "\n");
	}

	ctx.write("}");
}
