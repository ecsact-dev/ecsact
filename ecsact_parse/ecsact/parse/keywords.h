#ifndef ECSACT_PARSE_KEYWORDS_H
#define ECSACT_PARSE_KEYWORDS_H

#include "ecsact/runtime/definitions.h"

#define ECSACT_PARSE_KW_MAIN_PACKAGE "main package"
#define ECSACT_PARSE_KW_PACKAGE "package"
#define ECSACT_PARSE_KW_IMPORT "import"
#define ECSACT_PARSE_KW_COMPONENT "component"
#define ECSACT_PARSE_KW_TRANSIENT "transient"
#define ECSACT_PARSE_KW_SYSTEM "system"
#define ECSACT_PARSE_KW_ACTION "action"
#define ECSACT_PARSE_KW_ENUM "enum"

#define ECSACT_PARSE_KW_READONLY "readonly"
#define ECSACT_PARSE_KW_READWRITE "readwrite"
#define ECSACT_PARSE_KW_WRITEONLY "writeonly"
#define ECSACT_PARSE_KW_ADDS "adds"
#define ECSACT_PARSE_KW_REMOVES "removes"
#define ECSACT_PARSE_KW_EXCLUDE "exclude"
#define ECSACT_PARSE_KW_INCLUDE "include"
#define ECSACT_PARSE_KW_REQUIRED "required"
#define ECSACT_PARSE_KW_OPTIONAL "optional"
#define ECSACT_PARSE_KW_GENERATES "generates"

#define ECSACT_PARSE_KW_I8 "i8"
#define ECSACT_PARSE_KW_U8 "u8"
#define ECSACT_PARSE_KW_I16 "i16"
#define ECSACT_PARSE_KW_U16 "u16"
#define ECSACT_PARSE_KW_I32 "i32"
#define ECSACT_PARSE_KW_U32 "u32"
#define ECSACT_PARSE_KW_F32 "f32"
#define ECSACT_PARSE_KW_I64 "i64"
#define ECSACT_PARSE_KW_U64 "u64"
#define ECSACT_PARSE_KW_F64 "f64"
#define ECSACT_PARSE_KW_BOOL "bool"
#define ECSACT_PARSE_KW_ENTITY "entity"

#ifdef __cplusplus
#	include <string>
#	include <string_view>
#	include <array>

namespace ecsact::parse {

constexpr auto builtin_type_to_keyword_string(ecsact_builtin_type type)
	-> std::string_view {
	switch(type) {
		case ECSACT_BOOL:
			return ECSACT_PARSE_KW_BOOL;
		case ECSACT_I8:
			return ECSACT_PARSE_KW_I8;
		case ECSACT_U8:
			return ECSACT_PARSE_KW_U8;
		case ECSACT_I16:
			return ECSACT_PARSE_KW_I16;
		case ECSACT_U16:
			return ECSACT_PARSE_KW_U16;
		case ECSACT_I32:
			return ECSACT_PARSE_KW_I32;
		case ECSACT_U32:
			return ECSACT_PARSE_KW_U32;
		case ECSACT_F32:
			return ECSACT_PARSE_KW_F32;
		case ECSACT_I64:
			return ECSACT_PARSE_KW_I64;
		case ECSACT_U64:
			return ECSACT_PARSE_KW_U64;
		case ECSACT_F64:
			return ECSACT_PARSE_KW_F64;
		case ECSACT_ENTITY_TYPE:
			return ECSACT_PARSE_KW_ENTITY;
	}

	return "unknown";
}

constexpr auto top_level_keywords = std::array{
	std::string_view{ECSACT_PARSE_KW_MAIN_PACKAGE},
	std::string_view{ECSACT_PARSE_KW_PACKAGE},
	std::string_view{ECSACT_PARSE_KW_IMPORT},
	std::string_view{ECSACT_PARSE_KW_COMPONENT},
	std::string_view{ECSACT_PARSE_KW_TRANSIENT},
	std::string_view{ECSACT_PARSE_KW_SYSTEM},
	std::string_view{ECSACT_PARSE_KW_ACTION},
	std::string_view{ECSACT_PARSE_KW_ENUM},
};

constexpr auto cap_keywords = std::array{
	std::string_view{ECSACT_PARSE_KW_READONLY},
	std::string_view{ECSACT_PARSE_KW_READWRITE},
	std::string_view{ECSACT_PARSE_KW_WRITEONLY},
	std::string_view{ECSACT_PARSE_KW_ADDS},
	std::string_view{ECSACT_PARSE_KW_REMOVES},
	std::string_view{ECSACT_PARSE_KW_EXCLUDE},
	std::string_view{ECSACT_PARSE_KW_INCLUDE},
	std::string_view{ECSACT_PARSE_KW_REQUIRED},
	std::string_view{ECSACT_PARSE_KW_GENERATES},
};

constexpr auto generates_keywords = std::array{
	std::string_view{ECSACT_PARSE_KW_REQUIRED},
	std::string_view{ECSACT_PARSE_KW_OPTIONAL},
};

constexpr auto type_keywords = std::array{
	std::string_view{ECSACT_PARSE_KW_BOOL},
	std::string_view{ECSACT_PARSE_KW_I8},
	std::string_view{ECSACT_PARSE_KW_U8},
	std::string_view{ECSACT_PARSE_KW_I16},
	std::string_view{ECSACT_PARSE_KW_U16},
	std::string_view{ECSACT_PARSE_KW_I32},
	std::string_view{ECSACT_PARSE_KW_U32},
	std::string_view{ECSACT_PARSE_KW_I64},
	std::string_view{ECSACT_PARSE_KW_U64},
	std::string_view{ECSACT_PARSE_KW_F32},
	std::string_view{ECSACT_PARSE_KW_F64},
	std::string_view{ECSACT_PARSE_KW_ENTITY},
};

} // namespace ecsact::parse
#endif

#endif // ECSACT_PARSE_KEYWORDS_H
