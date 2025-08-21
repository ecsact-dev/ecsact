#pragma once

#include <string>
#include "ecsact/runtime/definitions.h"

namespace ecsact::csharp_lang_support {

constexpr auto csharp_type_str(ecsact_builtin_type type) {
	switch(type) {
		case ECSACT_BOOL:
			return "global::System.Boolean";
		case ECSACT_I8:
			return "global::System.SByte";
		case ECSACT_U8:
			return "global::System.Byte";
		case ECSACT_I16:
			return "global::System.Int16";
		case ECSACT_U16:
			return "global::System.UInt16";
		case ECSACT_I32:
			return "global::System.Int32";
		case ECSACT_U32:
			return "global::System.UInt32";
		case ECSACT_F32:
			return "global::System.Single";
		case ECSACT_ENTITY_TYPE:
			return "global::System.Int32";
	}
}

inline std::string csharp_field_attribute_str(ecsact_field_type field_type) {
	using namespace std::string_literals;

	auto introp_services = "global::System.Runtime.InteropServices"s;
	auto unmanaged_type = [&](auto t) -> std::string {
		return introp_services + ".UnmanagedType."s + t;
	};
	auto marshal_as = [&]<typename... T>(T... args) {
		return "[" + introp_services + ".MarshalAs("s + (args + ...) + ")]"s;
	};

	if(field_type.kind == ECSACT_TYPE_KIND_BUILTIN) {
		if(field_type.type.builtin == ECSACT_BOOL && field_type.length == 1) {
			return marshal_as(unmanaged_type("I1"));
		} else if(field_type.type.builtin == ECSACT_BOOL && field_type.length > 1) {
			return marshal_as(
				unmanaged_type("ByValArray"),
				", ArraySubType=" + unmanaged_type("I1"),
				", SizeConst=" + std::to_string(field_type.length)
			);
		}
	}

	if(field_type.length > 1) {
		return marshal_as(
			unmanaged_type("ByValArray"),
			", SizeConst=" + std::to_string(field_type.length)
		);
	}

	return "";
}

} // namespace ecsact::csharp_lang_support
