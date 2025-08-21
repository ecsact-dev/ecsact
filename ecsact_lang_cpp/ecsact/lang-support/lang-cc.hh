#pragma once

#include <string>
#include "ecsact/runtime/common.h"
#include "ecsact/runtime/meta.h"
#include "ecsact/runtime/definitions.h"

namespace ecsact::cc_lang_support {

/**
 * Convert strings like foo.bar to foo::bar
 */
inline std::string cpp_identifier(std::string str) {
	auto idx = str.find_first_of('.');
	while(idx != std::string::npos) {
		auto replace_length = 1;
		auto next_idx = str.find_first_of('.', idx + 1);
		while(next_idx != std::string::npos && next_idx == idx + 1) {
			next_idx = str.find_first_of('.', next_idx + 1);
			replace_length += 1;
		}

		str.replace(idx, replace_length, "::");

		idx = str.find_first_of('.');
	}

	return str;
}

/**
 * Convert strings like foo.bar to foo__bar
 */
inline std::string c_identifier(std::string str) {
	auto idx = str.find_first_of('.');
	while(idx != std::string::npos) {
		auto replace_length = 1;
		auto next_idx = str.find_first_of('.', idx + 1);
		while(next_idx != std::string::npos && next_idx == idx + 1) {
			next_idx = str.find_first_of('.', next_idx + 1);
			replace_length += 1;
		}

		str.replace(idx, replace_length, "__");

		idx = str.find_first_of('.');
	}

	return str;
}

constexpr auto cpp_type_str(ecsact_builtin_type type) {
	switch(type) {
		case ECSACT_BOOL:
			return "bool";
		case ECSACT_I8:
			return "int8_t";
		case ECSACT_U8:
			return "uint8_t";
		case ECSACT_I16:
			return "int16_t";
		case ECSACT_U16:
			return "uint16_t";
		case ECSACT_I32:
			return "int32_t";
		case ECSACT_U32:
			return "uint32_t";
		case ECSACT_F32:
			return "float";
		case ECSACT_ENTITY_TYPE:
			return "::ecsact_entity_id";
	}
}

ECSACT_ALWAYS_INLINE auto cpp_field_type_name( //
	ecsact_field_type field_type
) -> std::string {
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

constexpr auto c_type_str(ecsact_builtin_type type) {
	if(type == ECSACT_ENTITY_TYPE) {
		return "ecsact_entity_id";
	} else {
		return cpp_type_str(type);
	}
}

template<typename SystemID>
inline std::string anonymous_system_name(SystemID id) {
	auto sys_id = ecsact_id_cast<ecsact_system_like_id>(id);
	return "AnonymousSystem_" + std::to_string((int)sys_id);
}

} // namespace ecsact::cc_lang_support
