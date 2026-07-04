#include "ecsact/runtime/dynamic.h"
#include "ecsact/runtime/meta.h"

#include <map>
#include <print>
#include <string>
#include <atomic>
#include <vector>
#include <limits>
#include <variant>
#include <cassert>
#include <optional>
#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include "ecsact_interpret/parse-resolver-runtime/lifecycle.hh"

using ecsact::interpret::details::reset_lifecycle;
using ecsact::interpret::details::trigger_on_destroy;

template<typename T, typename... Ts>
struct is_any : std::disjunction<std::is_same<T, Ts>...> {};

template<typename T, typename... Ts>
inline constexpr bool is_any_v = is_any<T, Ts...>::value;

static std::atomic<int32_t> last_id_val =
	static_cast<int32_t>(ECSACT_BUILTIN_PACKAGE_MAX_ID) + 1;

template<typename T>
static T next_id() {
	return static_cast<T>(++last_id_val);
}

struct composite {
	struct field {
		std::string       name;
		ecsact_field_type type;
	};

	std::map<ecsact_field_id, field> fields;

	inline ecsact_field_id next_field_id() {
		return static_cast<ecsact_field_id>(fields.size());
	}
};

struct comp_def : composite {
	std::string           name;
	ecsact_component_type comp_type;
};

struct trans_def : composite {
	std::string name;
};

struct system_like {
	struct cap_entry {
		ecsact_system_capability cap;
	};

	struct gen_entry {
		ecsact_component_id    comp_id;
		ecsact_system_generate flag;
	};

	using caps_t = std::unordered_map< //
		ecsact_component_like_id,
		cap_entry>;
	caps_t caps;

	using generates_t = std::unordered_map<
		ecsact_system_generates_id,
		std::unordered_map<ecsact_component_id, ecsact_system_generate>>;
	generates_t generates;

	using notify_settings_t = std::unordered_map< //
		ecsact_component_like_id,
		ecsact_system_notify_setting>;
	notify_settings_t notify_settings;

	ecsact_system_like_id parent_system_id = (ecsact_system_like_id)-1;

	/** in execution order */
	std::vector<std::variant<ecsact_system_like_id, ecsact_cluster_id>>
		execution_order;

	ecsact_parallel_execution parallel_execution = {};

	std::vector<std::vector<ecsact_system_like_id>> execution_batches;
};

struct cluster_def {
	std::string                        name;
	std::vector<ecsact_system_like_id> systems;
};

struct action_def : composite, system_like {
	std::string name;
};

struct system_def : system_like {
	std::string name;
	int32_t     lazy_iteration_rate = 0;
};

struct enum_value {
	std::string name;
	int32_t     value;
};

class enum_def {
	int32_t _last_enum_value_id = -1;

public:
	std::string                                name;
	std::map<ecsact_enum_value_id, enum_value> enum_values;

	inline ecsact_enum_value_id next_enum_value_id() {
		return static_cast<ecsact_enum_value_id>(++_last_enum_value_id);
	}
};

using any_def = std::variant<comp_def, action_def, system_def>;

struct package_def {
	std::string name;
	bool        main;
	std::string source_file_path;

	std::vector<ecsact_package_id>   dependencies;
	std::vector<ecsact_system_id>    systems;
	std::vector<ecsact_action_id>    actions;
	std::vector<ecsact_component_id> components;
	std::vector<ecsact_transient_id> transients;
	std::vector<ecsact_enum_id>      enums;

	/** in execution order */
	std::vector<std::variant<ecsact_system_like_id, ecsact_cluster_id>>
		execution_order;

	std::vector<std::vector<ecsact_system_like_id>> execution_batches;
};

// clang-format off
static const auto builtin_package_defs = std::unordered_map<ecsact_package_id, package_def>{
	{
		ECSACT_BUILTIN_PKG_NOTIFY_ID,
		package_def{
			.name = "ecsact.notify",
			.main = false,
			.source_file_path = "<builtin package: ecsact.notify>",
		},
	},
};
//clang-format on

static std::unordered_map<ecsact_decl_id, std::string>       full_names;
static std::unordered_map<ecsact_package_id, package_def>    package_defs = builtin_package_defs;
static std::unordered_map<ecsact_decl_id, ecsact_package_id> def_owner_map;
static std::unordered_map<ecsact_component_id, comp_def>     comp_defs;
static std::unordered_map<ecsact_transient_id, trans_def>    trans_defs;
static std::unordered_map<ecsact_system_id, system_def>      sys_defs;
static std::unordered_map<ecsact_action_id, action_def>      act_defs;
static std::unordered_map<ecsact_enum_id, enum_def>          enum_defs;
static std::unordered_map<ecsact_cluster_id, cluster_def>    cluster_defs;
static std::optional<ecsact_package_id>                      main_package_id;

void ecsact_interpret_reset() {
	full_names.clear();
	package_defs = builtin_package_defs;
	def_owner_map.clear();
	comp_defs.clear();
	trans_defs.clear();
	sys_defs.clear();
	act_defs.clear();
	enum_defs.clear();
	cluster_defs.clear();
	main_package_id = std::nullopt;
	reset_lifecycle();
}

template<typename T>
static ecsact_package_id owner_package_id(T id) {
	ecsact_decl_id decl_id = ecsact_id_cast<ecsact_decl_id>(id);
	auto           itr = def_owner_map.find(decl_id);
	if(itr == def_owner_map.end()) {
		return ECSACT_INVALID_ID(package);
	}
	return itr->second;
}

static package_def& get_package_def(ecsact_package_id id) {
	auto itr = package_defs.find(id);
	if(itr == package_defs.end()) {
		throw std::runtime_error("Invalid package_id");
	}
	return itr->second;
}

static void set_package_owner(ecsact_decl_id id, ecsact_package_id owner) {
	def_owner_map[id] = owner;
}

static system_like& get_system_like(ecsact_system_like_id id) {
	auto sys_itr = sys_defs.find(static_cast<ecsact_system_id>(id));
	if(sys_itr != sys_defs.end()) {
		return sys_itr->second;
	}
	auto act_itr = act_defs.find(static_cast<ecsact_action_id>(id));
	if(act_itr != act_defs.end()) {
		return act_itr->second;
	}

	throw std::runtime_error("Invalid system_like_id");
}

static composite& get_composite(ecsact_composite_id id) {
	auto comp_itr = comp_defs.find(static_cast<ecsact_component_id>(id));
	if(comp_itr != comp_defs.end()) {
		return comp_itr->second;
	}
	auto trans_itr = trans_defs.find(static_cast<ecsact_transient_id>(id));
	if(trans_itr != trans_defs.end()) {
		return trans_itr->second;
	}
	auto act_itr = act_defs.find(static_cast<ecsact_action_id>(id));
	if(act_itr != act_defs.end()) {
		return act_itr->second;
	}

	throw std::runtime_error("Invalid composite_id");
}

auto ecsact_create_package(
	bool        main_package,
	const char* package_name,
	int32_t     package_name_len
) -> ecsact_package_id {
	auto id = next_id<ecsact_package_id>();
	auto name = std::string(package_name, package_name_len);

	package_defs[id] = {
		.name = name,
		.main = main_package,
	};

	if(main_package) {
		main_package_id = id;
	}

	return id;
}

auto ecsact_add_dependency(
	ecsact_package_id target,
	ecsact_package_id dependency
) -> void {
	auto pkg_itr = package_defs.find(target);
	if(pkg_itr != package_defs.end()) {
		auto& deps = pkg_itr->second.dependencies;
		auto  itr = std::find(deps.begin(), deps.end(), dependency);
		if(itr == deps.end()) {
			deps.push_back(dependency);
		}
	}
}

void ecsact_remove_dependency(
	ecsact_package_id target,
	ecsact_package_id dependency
) {
	auto& deps = get_package_def(target).dependencies;
	deps.erase(std::remove(deps.begin(), deps.end(), dependency), deps.end());
}

void ecsact_destroy_package(ecsact_package_id package_id) {
	if(main_package_id == package_id) {
		main_package_id = std::nullopt;
	}

	package_defs.erase(package_id);
	auto owner_itr = def_owner_map.begin();
	while(owner_itr != def_owner_map.end()) {
		if(owner_itr->second == package_id) {
			auto decl_id = owner_itr->first;
			full_names.erase(decl_id);
			comp_defs.erase(static_cast<ecsact_component_id>(decl_id));
			trans_defs.erase(static_cast<ecsact_transient_id>(decl_id));
			sys_defs.erase(static_cast<ecsact_system_id>(decl_id));
			act_defs.erase(static_cast<ecsact_action_id>(decl_id));
			enum_defs.erase(static_cast<ecsact_enum_id>(decl_id));
			cluster_defs.erase(static_cast<ecsact_cluster_id>(decl_id));
			owner_itr = def_owner_map.erase(owner_itr);
		} else {
			++owner_itr;
		}
	}

	trigger_on_destroy(package_id);
}

int32_t ecsact_meta_count_packages() {
	return static_cast<int32_t>(package_defs.size());
}

auto ecsact_meta_get_package_ids(
	int32_t            max_package_count,
	ecsact_package_id* out_package_ids,
	int32_t*           out_package_count
) -> void {
	auto itr = package_defs.begin();
	for(int i = 0; max_package_count > i; ++i, ++itr) {
		if(itr == package_defs.end()) {
			break;
		}
		out_package_ids[i] = itr->first;
	}

	if(out_package_count != nullptr) {
		*out_package_count = static_cast<int32_t>(package_defs.size());
	}
}

const char* ecsact_meta_package_name(ecsact_package_id package_id) {
	auto itr = package_defs.find(package_id);
	if(itr != package_defs.end()) {
		return itr->second.name.c_str();
	}

	return nullptr;
}

ecsact_component_id ecsact_create_component(
	ecsact_package_id owner,
	const char*       component_name,
	int32_t           component_name_len
) {
	auto& pkg_def = get_package_def(owner);
	auto  comp_id = next_id<ecsact_component_id>();
	auto  decl_id = ecsact_id_cast<ecsact_decl_id>(comp_id);
	pkg_def.components.push_back(comp_id);
	set_package_owner(decl_id, owner);
	auto& def = comp_defs[comp_id];
	def.name = std::string(component_name, component_name_len);
	def.comp_type = ECSACT_COMPONENT_TYPE_NONE;
	full_names[decl_id] = pkg_def.name + "." + def.name;

	return comp_id;
}

ecsact_transient_id ecsact_create_transient(
	ecsact_package_id owner,
	const char*       transient_name,
	int32_t           transient_name_len
) {
	auto& pkg_def = get_package_def(owner);
	auto  trans_id = next_id<ecsact_transient_id>();
	auto  decl_id = ecsact_id_cast<ecsact_decl_id>(trans_id);
	pkg_def.transients.push_back(trans_id);
	set_package_owner(decl_id, owner);
	auto& def = trans_defs[trans_id];
	def.name = std::string(transient_name, transient_name_len);
	full_names[decl_id] = pkg_def.name + "." + def.name;

	return trans_id;
}

ecsact_system_id ecsact_create_system(
	ecsact_package_id owner,
	const char*       system_name,
	int32_t           system_name_len
) {
	auto&      pkg_def = get_package_def(owner);
	const auto sys_id = next_id<ecsact_system_id>();
	const auto decl_id = ecsact_id_cast<ecsact_decl_id>(sys_id);
	const auto sys_like_id = static_cast<ecsact_system_like_id>(sys_id);
	pkg_def.systems.push_back(sys_id);
	pkg_def.execution_order.push_back(sys_like_id);
	set_package_owner(decl_id, owner);
	auto& def = sys_defs[sys_id];
	def.name = std::string(system_name, system_name_len);
	if(def.name.empty()) {
		full_names[decl_id] = "";
	} else {
		full_names[decl_id] = pkg_def.name + "." + def.name;
	}

	return sys_id;
}

ecsact_action_id ecsact_create_action(
	ecsact_package_id owner,
	const char*       action_name,
	int32_t           action_name_len
) {
	auto&      pkg_def = get_package_def(owner);
	const auto act_id = next_id<ecsact_action_id>();
	const auto decl_id = ecsact_id_cast<ecsact_decl_id>(act_id);
	const auto sys_like_id = static_cast<ecsact_system_like_id>(act_id);
	pkg_def.actions.push_back(act_id);
	pkg_def.execution_order.push_back(sys_like_id);
	set_package_owner(decl_id, owner);
	auto& def = act_defs[act_id];
	def.name = std::string(action_name, action_name_len);
	full_names[decl_id] = pkg_def.name + "." + def.name;

	return act_id;
}

ecsact_enum_id ecsact_create_enum(
	ecsact_package_id owner,
	const char*       enum_name,
	int32_t           enum_name_len
) {
	auto& pkg_def = get_package_def(owner);
	auto  enum_id = next_id<ecsact_enum_id>();
	auto& def = enum_defs[enum_id];
	def.name = std::string(enum_name, enum_name_len);
	pkg_def.enums.push_back(enum_id);
	set_package_owner(ecsact_id_cast<ecsact_decl_id>(enum_id), owner);

	return enum_id;
}

ecsact_enum_value_id ecsact_add_enum_value(
	ecsact_enum_id enum_id,
	int32_t        value,
	const char*    value_name,
	int32_t        value_name_len
) {
	auto& def = enum_defs.at(enum_id);
	auto  enum_value_id = def.next_enum_value_id();
	auto& enum_value = def.enum_values[enum_value_id];
	enum_value.name = std::string(value_name, value_name_len);
	enum_value.value = value;
	return enum_value_id;
}

int32_t ecsact_meta_count_systems(ecsact_package_id package_id) {
	auto& pkg_def = get_package_def(package_id);
	return static_cast<int32_t>(pkg_def.systems.size());
}

void ecsact_meta_get_system_ids(
	ecsact_package_id package_id,
	int32_t           max_system_count,
	ecsact_system_id* out_system_ids,
	int32_t*          out_system_count
) {
	auto& pkg_def = get_package_def(package_id);

	auto itr = pkg_def.systems.begin();
	for(int32_t i = 0; max_system_count > i && itr != pkg_def.systems.end();
			++i) {
		out_system_ids[i] = *itr;
		++itr;
	}

	if(out_system_count != nullptr) {
		*out_system_count = static_cast<int32_t>(pkg_def.systems.size());
	}
}

int32_t ecsact_meta_count_actions(ecsact_package_id package_id) {
	auto& pkg_def = get_package_def(package_id);
	return static_cast<int32_t>(pkg_def.actions.size());
}

void ecsact_meta_get_action_ids(
	ecsact_package_id package_id,
	int32_t           max_action_count,
	ecsact_action_id* out_action_ids,
	int32_t*          out_action_count
) {
	auto& pkg_def = get_package_def(package_id);
	auto  itr = pkg_def.actions.begin();
	for(int32_t i = 0; max_action_count > i && itr != pkg_def.actions.end();
			++i) {
		out_action_ids[i] = *itr;
		++itr;
	}

	if(out_action_count != nullptr) {
		*out_action_count = static_cast<int32_t>(pkg_def.actions.size());
	}
}

int32_t ecsact_meta_count_components(ecsact_package_id package_id) {
	auto& pkg_def = get_package_def(package_id);
	return static_cast<int32_t>(pkg_def.components.size());
}

int32_t ecsact_meta_count_transients(ecsact_package_id package_id) {
	auto& pkg_def = get_package_def(package_id);
	return static_cast<int32_t>(pkg_def.transients.size());
}

const char* ecsact_meta_component_name(ecsact_component_id comp_id) {
	return comp_defs.at(comp_id).name.c_str();
}

const char* ecsact_meta_transient_name(ecsact_transient_id trans) {
	return trans_defs.at(trans).name.c_str();
}

const char* ecsact_meta_system_name(ecsact_system_id sys_id) {
	auto& name = sys_defs.at(sys_id).name;
	if(name.empty()) {
		return "";
	}
	return name.c_str();
}

const char* ecsact_meta_action_name(ecsact_action_id act_id) {
	return act_defs.at(act_id).name.c_str();
}

int32_t ecsact_meta_count_enums(ecsact_package_id package_id) {
	auto& pkg_def = get_package_def(package_id);
	return static_cast<int32_t>(pkg_def.enums.size());
}

void ecsact_meta_get_enum_ids(
	ecsact_package_id package_id,
	int32_t           max_enum_count,
	ecsact_enum_id*   out_enum_ids,
	int32_t*          out_enum_count
) {
	auto& pkg_def = get_package_def(package_id);

	auto itr = pkg_def.enums.begin();
	for(int i = 0; max_enum_count > i && itr != pkg_def.enums.end(); ++i) {
		out_enum_ids[i] = *itr;
		++itr;
	}

	if(out_enum_count) {
		*out_enum_count = static_cast<int32_t>(pkg_def.enums.size());
	}
}

ecsact_builtin_type ecsact_meta_enum_storage_type(ecsact_enum_id enum_id) {
	using std::numeric_limits;

	auto&   def = enum_defs.at(enum_id);
	int32_t min_value = numeric_limits<int32_t>::max();
	int32_t max_value = numeric_limits<int32_t>::min();
	for(auto& [_, enum_value] : def.enum_values) {
		if(enum_value.value > max_value) {
			max_value = enum_value.value;
		}

		if(enum_value.value < min_value) {
			min_value = enum_value.value;
		}
	}

	if(min_value >= 0) {
		if(max_value <= numeric_limits<uint8_t>::max()) {
			return ECSACT_U8;
		}
		if(max_value <= numeric_limits<uint16_t>::max()) {
			return ECSACT_U16;
		}
		return ECSACT_U32;
	} else {
		if(
			min_value >= numeric_limits<int8_t>::min() &&
			max_value <= numeric_limits<int8_t>::max()
		) {
			return ECSACT_I8;
		}
		if(
			min_value >= numeric_limits<int16_t>::min() &&
			max_value <= numeric_limits<int16_t>::max()
		) {
			return ECSACT_I16;
		}
		return ECSACT_I32;
	}
}

int32_t ecsact_meta_count_enum_values(ecsact_enum_id enum_id) {
	auto& def = enum_defs.at(enum_id);
	return static_cast<int32_t>(def.enum_values.size());
}

void ecsact_meta_get_enum_value_ids(
	ecsact_enum_id        enum_id,
	int32_t               max_enum_value_count,
	ecsact_enum_value_id* out_enum_value_ids,
	int32_t*              out_enum_value_count
) {
	auto& def = enum_defs.at(enum_id);

	auto itr = def.enum_values.begin();
	for(int i = 0; max_enum_value_count > i && itr != def.enum_values.end();
			++i) {
		out_enum_value_ids[i] = itr->first;
		++itr;
	}

	if(out_enum_value_count) {
		*out_enum_value_count = static_cast<int32_t>(def.enum_values.size());
	}
}

const char* ecsact_meta_enum_name(ecsact_enum_id enum_id) {
	return enum_defs.at(enum_id).name.c_str();
}

const char* ecsact_meta_enum_value_name(
	ecsact_enum_id       enum_id,
	ecsact_enum_value_id enum_value_id
) {
	return enum_defs.at(enum_id).enum_values.at(enum_value_id).name.c_str();
}

int32_t ecsact_meta_enum_value(
	ecsact_enum_id       enum_id,
	ecsact_enum_value_id enum_value_id
) {
	return enum_defs.at(enum_id).enum_values.at(enum_value_id).value;
}

const char* ecsact_meta_registry_name(ecsact_registry_id) {
	return "";
}

ecsact_package_id ecsact_meta_main_package() {
	return main_package_id.value_or((ecsact_package_id)-1);
}

void ecsact_meta_get_component_ids(
	ecsact_package_id    package_id,
	int32_t              max_component_count,
	ecsact_component_id* out_component_ids,
	int32_t*             out_component_count
) {
	auto& pkg_def = get_package_def(package_id);

	auto itr = pkg_def.components.begin();
	for(int i = 0; max_component_count > i && itr != pkg_def.components.end();
			++i) {
		out_component_ids[i] = *itr;
		++itr;
	}

	if(out_component_count) {
		*out_component_count = static_cast<int32_t>(pkg_def.components.size());
	}
}

void ecsact_meta_get_transient_ids(
	ecsact_package_id    package_id,
	int32_t              max_transient_count,
	ecsact_transient_id* out_transient_ids,
	int32_t*             out_transient_count
) {
	auto& pkg_def = get_package_def(package_id);

	auto itr = pkg_def.transients.begin();
	for(int i = 0; max_transient_count > i && itr != pkg_def.transients.end();
			++i) {
		out_transient_ids[i] = *itr;
		++itr;
	}

	if(out_transient_count) {
		*out_transient_count = static_cast<int32_t>(pkg_def.transients.size());
	}
}

int32_t ecsact_meta_count_fields(ecsact_composite_id composite_id) {
	auto& def = get_composite(composite_id);
	return static_cast<int32_t>(def.fields.size());
}

void ecsact_meta_get_field_ids(
	ecsact_composite_id composite_id,
	int32_t             max_field_count,
	ecsact_field_id*    out_field_ids,
	int32_t*            out_field_count
) {
	auto& def = get_composite(composite_id);

	auto itr = def.fields.begin();
	for(int i = 0; max_field_count > i && itr != def.fields.end(); ++i) {
		out_field_ids[i] = itr->first;
		++itr;
	}

	if(out_field_count) {
		*out_field_count = static_cast<int32_t>(def.fields.size());
	}
}

const char* ecsact_meta_field_name(
	ecsact_composite_id composite_id,
	ecsact_field_id     field_id
) {
	auto& def = get_composite(composite_id);
	return def.fields.at(field_id).name.c_str();
}

ecsact_field_type ecsact_meta_field_type(
	ecsact_composite_id composite_id,
	ecsact_field_id     field_id
) {
	auto& def = get_composite(composite_id);
	return def.fields.at(field_id).type;
}

static int32_t builtin_type_size(ecsact_builtin_type type) {
	switch(type) {
		case ECSACT_BOOL:
			return 1;
		case ECSACT_I8:
		case ECSACT_U8:
			return 1;
		case ECSACT_I16:
		case ECSACT_U16:
			return 2;
		case ECSACT_I32:
		case ECSACT_U32:
		case ECSACT_F32:
		case ECSACT_ENTITY_TYPE:
			return 4;
	}
	return 0;
}

static int32_t field_type_size(ecsact_field_type type) {
	int32_t base_size = 0;
	if(type.kind == ECSACT_TYPE_KIND_BUILTIN) {
		base_size = builtin_type_size(type.type.builtin);
	} else if(type.kind == ECSACT_TYPE_KIND_ENUM) {
		base_size =
			builtin_type_size(ecsact_meta_enum_storage_type(type.type.enum_id));
	}

	return base_size * type.length;
}

int32_t ecsact_meta_field_offset(
	ecsact_composite_id composite_id,
	ecsact_field_id     field_id
) {
	auto&   def = get_composite(composite_id);
	int32_t offset = 0;
	for(auto const& [id, field] : def.fields) {
		if(id == field_id) {
			return offset;
		}
		offset += field_type_size(field.type);
	}
	return -1;
}

ecsact_field_id ecsact_add_field(
	ecsact_composite_id composite_id,
	ecsact_field_type   field_type,
	const char*         field_name,
	int32_t             field_name_len
) {
	auto& def = get_composite(composite_id);
	auto  id = def.next_field_id();
	auto& field = def.fields[id];
	field.name = std::string(field_name, field_name_len);
	field.type = field_type;
	return id;
}

void ecsact_remove_field(
	ecsact_composite_id composite_id,
	ecsact_field_id     field_id
) {
	auto& def = get_composite(composite_id);
	def.fields.erase(field_id);
}

void ecsact_destroy_component(ecsact_component_id component_id) {
	auto decl_id = ecsact_id_cast<ecsact_decl_id>(component_id);
	auto owner = owner_package_id(component_id);
	if((int32_t)owner != -1) {
		auto& pkg = get_package_def(owner);
		pkg.components.erase(
			std::remove(pkg.components.begin(), pkg.components.end(), component_id),
			pkg.components.end()
		);
	}
	comp_defs.erase(component_id);
	def_owner_map.erase(decl_id);
	full_names.erase(decl_id);
}

void ecsact_destroy_transient(ecsact_transient_id transient_id) {
	auto decl_id = ecsact_id_cast<ecsact_decl_id>(transient_id);
	auto owner = owner_package_id(transient_id);
	if((int32_t)owner != -1) {
		auto& pkg = get_package_def(owner);
		pkg.transients.erase(
			std::remove(pkg.transients.begin(), pkg.transients.end(), transient_id),
			pkg.transients.end()
		);
	}
	trans_defs.erase(transient_id);
	def_owner_map.erase(decl_id);
	full_names.erase(decl_id);
}

void ecsact_destroy_enum(ecsact_enum_id enum_id) {
	auto decl_id = ecsact_id_cast<ecsact_decl_id>(enum_id);
	auto owner = owner_package_id(enum_id);
	if((int32_t)owner != -1) {
		auto& pkg = get_package_def(owner);
		pkg.enums.erase(
			std::remove(pkg.enums.begin(), pkg.enums.end(), enum_id),
			pkg.enums.end()
		);
	}
	enum_defs.erase(enum_id);
	def_owner_map.erase(decl_id);
	full_names.erase(decl_id);
}

void ecsact_remove_enum_value(
	ecsact_enum_id       enum_id,
	ecsact_enum_value_id value_id
) {
	auto& def = enum_defs.at(enum_id);
	def.enum_values.erase(value_id);
}

void ecsact_set_system_capability(
	ecsact_system_like_id    system_id,
	ecsact_component_like_id component_id,
	ecsact_system_capability capability
) {
	auto& def = get_system_like(system_id);
	def.caps[component_id] = {capability};
}

void ecsact_unset_system_capability(
	ecsact_system_like_id    system_id,
	ecsact_component_like_id component_id
) {
	auto& def = get_system_like(system_id);
	def.caps.erase(component_id);
}

int32_t ecsact_meta_system_capabilities_count(ecsact_system_like_id system_id) {
	auto& def = get_system_like(system_id);
	return static_cast<int32_t>(def.caps.size());
}

void ecsact_meta_system_capabilities(
	ecsact_system_like_id     system_id,
	int32_t                   max_capabilities_count,
	ecsact_component_like_id* out_capability_component_ids,
	ecsact_system_capability* out_capabilities,
	int32_t*                  out_capabilities_count
) {
	auto& def = get_system_like(system_id);
	auto  itr = def.caps.begin();
	for(int i = 0; max_capabilities_count > i; ++i) {
		if(i >= def.caps.size() || itr == def.caps.end()) {
			break;
		}

		out_capability_component_ids[i] = itr->first;
		out_capabilities[i] = itr->second.cap;

		++itr;
	}

	if(out_capabilities_count != nullptr) {
		*out_capabilities_count = static_cast<int32_t>(def.caps.size());
	}
}

void ecsact_remove_child_system(
	ecsact_system_like_id parent,
	ecsact_system_id      child
) {
	auto& child_def = sys_defs.at(child);
	if(child_def.parent_system_id != parent) {
		return;
	}

	auto& parent_def = get_system_like(parent);
	auto  itr = std::find_if(
		parent_def.execution_order.begin(),
		parent_def.execution_order.end(),
		[&](const auto& entry) {
			auto sys_id = std::get_if<ecsact_system_like_id>(&entry);
			return sys_id && *sys_id == static_cast<ecsact_system_like_id>(child);
		}
	);

	child_def.parent_system_id = (ecsact_system_like_id)-1;
	if(itr != parent_def.execution_order.end()) {
		auto& pkg_def = get_package_def(owner_package_id(parent));
		pkg_def.execution_order.push_back(
			static_cast<ecsact_system_like_id>(child)
		);
		parent_def.execution_order.erase(itr);
	}

	if(!child_def.name.empty()) {
		auto& pkg_def = get_package_def(owner_package_id(child));
		auto  child_decl_id = ecsact_id_cast<ecsact_decl_id>(child);
		full_names.at(child_decl_id) = pkg_def.name + "." + child_def.name;
	}
}

void ecsact_add_child_system(
	ecsact_system_like_id parent,
	ecsact_system_id      child
) {
	auto& child_def = sys_defs.at(child);
	auto& parent_def = get_system_like(parent);
	auto& pkg_def = get_package_def(owner_package_id(parent));

	if((int32_t)child_def.parent_system_id != -1) {
		ecsact_remove_child_system(parent, child);
	}

	child_def.parent_system_id = parent;
	parent_def.execution_order.push_back(
		static_cast<ecsact_system_like_id>(child)
	);

	auto iter = std::find_if(
		pkg_def.execution_order.begin(),
		pkg_def.execution_order.end(),
		[&](const auto& entry) {
			auto sys_id = std::get_if<ecsact_system_like_id>(&entry);
			return sys_id && *sys_id == static_cast<ecsact_system_like_id>(child);
		}
	);

	if(iter != pkg_def.execution_order.end()) {
		pkg_def.execution_order.erase(iter);
	}

	if(!child_def.name.empty()) {
		auto child_decl_id = ecsact_id_cast<ecsact_decl_id>(child);
		auto parent_decl_id = ecsact_id_cast<ecsact_decl_id>(parent);

		auto& child_full_name = full_names.at(child_decl_id);
		if(!full_names.at(parent_decl_id).empty()) {
			child_full_name = full_names.at(parent_decl_id);
		} else {
			child_full_name = pkg_def.name;
		}

		child_full_name += "." + child_def.name;
	}
}

void ecsact_set_package_source_file_path(
	ecsact_package_id package_id,
	const char*       source_file_path,
	int32_t           source_file_path_len
) {
	assert(source_file_path_len > 0);
	auto& def = get_package_def(package_id);

	def.source_file_path = std::string(source_file_path, source_file_path_len);
}

const char* ecsact_meta_package_file_path(ecsact_package_id package_id) {
	auto& def = get_package_def(package_id);
	return def.source_file_path.c_str();
}

int32_t ecsact_meta_count_dependencies(ecsact_package_id package_id) {
	auto itr = package_defs.find(package_id);
	if(itr != package_defs.end()) {
		return static_cast<int32_t>(itr->second.dependencies.size());
	}
	return 0;
}

void ecsact_meta_get_dependencies(
	ecsact_package_id  package_id,
	int32_t            max_dependency_count,
	ecsact_package_id* out_dependencies,
	int32_t*           out_dependency_count
) {
	auto itr = package_defs.find(package_id);
	if(itr == package_defs.end()) {
		if(out_dependency_count != nullptr) {
			*out_dependency_count = 0;
		}
		return;
	}

	auto& pkg = itr->second;
	auto  count = std::min(
		max_dependency_count,
		static_cast<int32_t>(pkg.dependencies.size())
	);

	for(int i = 0; count > i; ++i) {
		out_dependencies[i] = pkg.dependencies[i];
	}

	if(out_dependency_count != nullptr) {
		*out_dependency_count = static_cast<int32_t>(pkg.dependencies.size());
	}
}

const char* ecsact_meta_decl_full_name(ecsact_decl_id id) {
	auto itr = full_names.find(id);
	if(itr != full_names.end()) {
		return itr->second.c_str();
	}
	return "";
}

static bool collect_all_caps(
	ecsact_system_like_id system_id,
	std::unordered_map<ecsact_component_like_id, ecsact_system_capability>&
		out_caps
) {
	auto& sys_def = get_system_like(system_id);
	bool  independent = !sys_def.generates.empty() ||
		sys_def.parallel_execution == ECSACT_PAR_EXEC_DENY;

	for(auto const& [comp_id, cap_entry] : sys_def.caps) {
		out_caps[comp_id] = static_cast<ecsact_system_capability>(
			static_cast<uint32_t>(out_caps[comp_id]) |
			static_cast<uint32_t>(cap_entry.cap)
		);
	}

	for(auto& entry : sys_def.execution_order) {
		if(auto child_sys_id = std::get_if<ecsact_system_like_id>(&entry)) {
			if(collect_all_caps(*child_sys_id, out_caps)) {
				independent = true;
			}
		} else if(auto cluster_id_ptr = std::get_if<ecsact_cluster_id>(&entry)) {
			auto cluster_itr = cluster_defs.find(*cluster_id_ptr);
			if(cluster_itr != cluster_defs.end()) {
				for(auto sys_id : cluster_itr->second.systems) {
					if(collect_all_caps(sys_id, out_caps)) {
						independent = true;
					}
				}
			}
		}
	}

	auto assoc_count = ecsact_meta_system_assoc_count(system_id);
	if(assoc_count > 0) {
		std::vector<ecsact_system_assoc_id> assoc_ids(assoc_count);
		ecsact_meta_system_assoc_ids(
			system_id,
			assoc_count,
			assoc_ids.data(),
			nullptr
		);

		for(auto assoc_id : assoc_ids) {
			auto cap_count =
				ecsact_meta_system_assoc_capabilities_count(system_id, assoc_id);
			std::vector<ecsact_component_like_id> comp_ids(cap_count);
			std::vector<ecsact_system_capability> caps(cap_count);
			ecsact_meta_system_assoc_capabilities(
				system_id,
				assoc_id,
				cap_count,
				comp_ids.data(),
				caps.data(),
				nullptr
			);

			for(int i = 0; i < cap_count; ++i) {
				out_caps[comp_ids[i]] = static_cast<ecsact_system_capability>(
					static_cast<uint32_t>(out_caps[comp_ids[i]]) |
					static_cast<uint32_t>(caps[i])
				);
			}
		}
	}

	return independent;
}

static ecsact_execution_batches_error calculate_execution_batches(
	const std::vector<std::variant<ecsact_system_like_id, ecsact_cluster_id>>&
																									 execution_order,
	std::vector<std::vector<ecsact_system_like_id>>& out_batches
) {
	out_batches.clear();

	std::vector<ecsact_system_like_id>           current_batch;
	std::unordered_set<ecsact_component_like_id> batch_writers;
	std::unordered_set<ecsact_component_like_id> batch_readers;

	auto is_exclusive = [](ecsact_system_capability cap) -> bool {
		if((cap & ECSACT_SYS_CAP_WRITEONLY) != 0) {
			return true;
		}
		if((cap & ECSACT_SYS_CAP_ADDS) == ECSACT_SYS_CAP_ADDS) {
			return true;
		}
		if((cap & ECSACT_SYS_CAP_REMOVES) == ECSACT_SYS_CAP_REMOVES) {
			return true;
		}
		if((cap & ECSACT_SYS_CAP_STREAM_TOGGLE) != 0) {
			return true;
		}
		return false;
	};

	auto is_reader = [](ecsact_system_capability cap) -> bool {
		return (cap & ECSACT_SYS_CAP_READONLY) != 0;
	};

	auto is_structural = [](ecsact_system_capability cap) -> bool {
		if((cap & ECSACT_SYS_CAP_ADDS) == ECSACT_SYS_CAP_ADDS) {
			return true;
		}
		if((cap & ECSACT_SYS_CAP_REMOVES) == ECSACT_SYS_CAP_REMOVES) {
			return true;
		}
		if((cap & ECSACT_SYS_CAP_STREAM_TOGGLE) != 0) {
			return true;
		}
		return false;
	};

	auto finalize_batch = [&]() {
		if(!current_batch.empty()) {
			out_batches.push_back(std::move(current_batch));
			current_batch = {};
			batch_writers.clear();
			batch_readers.clear();
		}
	};

	auto are_mutually_exclusive =
		[&](ecsact_system_like_id a, ecsact_system_like_id b) {
			std::unordered_map<ecsact_component_like_id, ecsact_system_capability>
				a_caps;
			std::unordered_map<ecsact_component_like_id, ecsact_system_capability>
				b_caps;
			collect_all_caps(a, a_caps);
			collect_all_caps(b, b_caps);

			for(auto const& [comp_id, a_cap] : a_caps) {
				if(!b_caps.contains(comp_id)) {
					continue;
				}
				auto b_cap = b_caps.at(comp_id);

				auto is_inc = [](ecsact_system_capability cap) -> bool {
					if((cap & ECSACT_SYS_CAP_INCLUDE) != 0) {
						return true;
					}
					if((cap & ECSACT_SYS_CAP_OPTIONAL) != 0) {
						return false;
					}
					return (cap & (ECSACT_SYS_CAP_READONLY | ECSACT_SYS_CAP_WRITEONLY)) !=
						0;
				};

				bool a_inc = is_inc(a_cap);
				bool a_exc = (a_cap & ECSACT_SYS_CAP_EXCLUDE) != 0;
				bool b_inc = is_inc(b_cap);
				bool b_exc = (b_cap & ECSACT_SYS_CAP_EXCLUDE) != 0;

				if((a_inc && b_exc) || (a_exc && b_inc)) {
					return true;
				}
			}

			return false;
		};

	auto check_conflict = [&]( //
													ecsact_system_like_id system_id,
													const std::unordered_map<
														ecsact_component_like_id,
														ecsact_system_capability>& all_caps
												) -> ecsact_execution_batches_error {
		for(auto const& [comp_id, cap] : all_caps) {
			ecsact_system_like_id conflicting_sys_id =
				static_cast<ecsact_system_like_id>(-1);
			ecsact_system_capability conflicting_sys_cap = ECSACT_SYS_CAP_NONE;

			if(is_exclusive(cap)) {
				if(batch_readers.contains(comp_id) || batch_writers.contains(comp_id)) {
					for(auto other_sys_id : current_batch) {
						std::unordered_map<ecsact_component_like_id, ecsact_system_capability>
								 other_caps;
						collect_all_caps(other_sys_id, other_caps);
						if(other_caps.contains(comp_id)) {
							auto other_cap = other_caps.at(comp_id);
							if(is_reader(other_cap) || is_exclusive(other_cap)) {
								conflicting_sys_id = other_sys_id;
								conflicting_sys_cap = other_cap;
								break;
							}
						}
					}
				}
			} else if(is_reader(cap)) {
				if(batch_writers.contains(comp_id)) {
					for(auto other_sys_id : current_batch) {
						std::unordered_map<ecsact_component_like_id, ecsact_system_capability>
								 other_caps;
						collect_all_caps(other_sys_id, other_caps);
						if(other_caps.contains(comp_id)) {
							auto other_cap = other_caps.at(comp_id);
							if(is_exclusive(other_cap)) {
								conflicting_sys_id = other_sys_id;
								conflicting_sys_cap = other_cap;
								break;
							}
						}
					}
				}
			}

			if(static_cast<int32_t>(conflicting_sys_id) != -1) {
				bool structural =
					is_structural(cap) || is_structural(conflicting_sys_cap);
				if(structural || !are_mutually_exclusive(system_id, conflicting_sys_id)) {
					return {
						.system_id = system_id,
						.conflicting_system_id = conflicting_sys_id,
						.component_id = comp_id,
					};
				}
			}
		}

		return {
			.system_id = static_cast<ecsact_system_like_id>(-1),
			.conflicting_system_id = static_cast<ecsact_system_like_id>(-1),
			.component_id = static_cast<ecsact_component_like_id>(-1),
		};
	};

	for(auto const& entry : execution_order) {
		if(auto sys_id_ptr = std::get_if<ecsact_system_like_id>(&entry)) {
			auto sys_id = *sys_id_ptr;
			std::unordered_map<ecsact_component_like_id, ecsact_system_capability>
					 all_caps;
			bool independent = collect_all_caps(sys_id, all_caps);

			auto err = check_conflict(sys_id, all_caps);
			if(static_cast<int32_t>(err.system_id) == -1 && independent) {
				finalize_batch();
			} else if(static_cast<int32_t>(err.system_id) != -1) {
				finalize_batch();
			}

			current_batch.push_back(sys_id);
			for(auto const& [comp_id, cap] : all_caps) {
				if(is_exclusive(cap)) {
					batch_writers.insert(comp_id);
				}
				if(is_reader(cap)) {
					batch_readers.insert(comp_id);
				}
			}

			if(independent) {
				finalize_batch();
			}
		} else if(auto cluster_id_ptr = std::get_if<ecsact_cluster_id>(&entry)) {
			finalize_batch();
			auto cluster_def_itr = cluster_defs.find(*cluster_id_ptr);
			if(cluster_def_itr == cluster_defs.end()) {
				continue;
			}
			auto& cluster_def = cluster_def_itr->second;
			for(auto sys_id : cluster_def.systems) {
				std::unordered_map<ecsact_component_like_id, ecsact_system_capability>
						 all_caps;
				bool independent = collect_all_caps(sys_id, all_caps);

				auto err = check_conflict(sys_id, all_caps);
				if(static_cast<int32_t>(err.system_id) != -1 || independent) {
					return {
						.system_id = sys_id,
						.conflicting_system_id = err.conflicting_system_id,
						.component_id = err.component_id,
					};
				}

				current_batch.push_back(sys_id);
				for(auto const& [comp_id, cap] : all_caps) {
					if(is_exclusive(cap)) {
						batch_writers.insert(comp_id);
					}
					if(is_reader(cap)) {
						batch_readers.insert(comp_id);
					}
				}
			}
			finalize_batch();
		}
	}

	finalize_batch();
	return {
		.system_id = static_cast<ecsact_system_like_id>(-1),
		.conflicting_system_id = static_cast<ecsact_system_like_id>(-1),
		.component_id = static_cast<ecsact_component_like_id>(-1),
	};
}

int32_t ecsact_meta_count_execution_batches(ecsact_package_id package_id) {
	auto& pkg = get_package_def(package_id);
	if(pkg.execution_batches.empty()) {
		std::vector<std::variant<ecsact_system_like_id, ecsact_cluster_id>>
			all_execution_order;
		for(auto dep_id : pkg.dependencies) {
			auto& dep_pkg = get_package_def(dep_id);
			for(auto& entry : dep_pkg.execution_order) {
				all_execution_order.push_back(entry);
			}
		}
		for(auto& entry : pkg.execution_order) {
			all_execution_order.push_back(entry);
		}

		if(!all_execution_order.empty()) {
			calculate_execution_batches(all_execution_order, pkg.execution_batches);
		}
	}
	return static_cast<int32_t>(pkg.execution_batches.size());
}

void ecsact_meta_get_execution_batch(
	ecsact_package_id      package_id,
	int32_t                batch_index,
	int32_t                max_systems_count,
	ecsact_system_like_id* out_systems,
	int32_t*               out_systems_count
) {
	auto& pkg = get_package_def(package_id);
	if(pkg.execution_batches.empty()) {
		std::vector<std::variant<ecsact_system_like_id, ecsact_cluster_id>>
			all_execution_order;
		for(auto dep_id : pkg.dependencies) {
			auto& dep_pkg = get_package_def(dep_id);
			for(auto& entry : dep_pkg.execution_order) {
				all_execution_order.push_back(entry);
			}
		}
		for(auto& entry : pkg.execution_order) {
			all_execution_order.push_back(entry);
		}

		if(!all_execution_order.empty()) {
			calculate_execution_batches(all_execution_order, pkg.execution_batches);
		}
	}

	auto& batch = pkg.execution_batches.at(batch_index);
	auto  count = std::min(max_systems_count, static_cast<int32_t>(batch.size()));

	for(int32_t i = 0; count > i; ++i) {
		out_systems[i] = batch[i];
	}

	if(out_systems_count != nullptr) {
		*out_systems_count = static_cast<int32_t>(batch.size());
	}
}

int32_t ecsact_meta_count_system_execution_batches(
	ecsact_system_like_id system_id
) {
	auto& sys_def = get_system_like(system_id);
	if(sys_def.execution_batches.empty() && !sys_def.execution_order.empty()) {
		calculate_execution_batches(
			sys_def.execution_order,
			sys_def.execution_batches
		);
	}
	return static_cast<int32_t>(sys_def.execution_batches.size());
}

void ecsact_meta_get_system_execution_batch(
	ecsact_system_like_id  system_id,
	int32_t                batch_index,
	int32_t                max_systems_count,
	ecsact_system_like_id* out_systems,
	int32_t*               out_systems_count
) {
	auto& sys_def = get_system_like(system_id);
	if(sys_def.execution_batches.empty() && !sys_def.execution_order.empty()) {
		calculate_execution_batches(
			sys_def.execution_order,
			sys_def.execution_batches
		);
	}

	auto& batch = sys_def.execution_batches.at(batch_index);
	auto  count = std::min(max_systems_count, static_cast<int32_t>(batch.size()));

	for(int32_t i = 0; count > i; ++i) {
		out_systems[i] = batch[i];
	}

	if(out_systems_count != nullptr) {
		*out_systems_count = static_cast<int32_t>(batch.size());
	}
}

ecsact_execution_batches_error ecsact_check_execution_batches(
	ecsact_package_id package_id
) {
	auto pkg_itr = package_defs.find(package_id);
	if(pkg_itr == package_defs.end()) {
		return {
			.system_id = static_cast<ecsact_system_like_id>(-1),
			.conflicting_system_id = static_cast<ecsact_system_like_id>(-1),
			.component_id = static_cast<ecsact_component_like_id>(-1),
		};
	}

	auto& pkg = pkg_itr->second;
	if(pkg.execution_batches.empty()) {
		std::vector<std::variant<ecsact_system_like_id, ecsact_cluster_id>>
			all_execution_order;
		for(auto dep_id : pkg.dependencies) {
			auto dep_pkg_itr = package_defs.find(dep_id);
			if(dep_pkg_itr != package_defs.end()) {
				for(auto& entry : dep_pkg_itr->second.execution_order) {
					all_execution_order.push_back(entry);
				}
			}
		}
		for(auto& entry : pkg.execution_order) {
			all_execution_order.push_back(entry);
		}

		if(all_execution_order.empty()) {
			return {
				.system_id = static_cast<ecsact_system_like_id>(-1),
				.conflicting_system_id = static_cast<ecsact_system_like_id>(-1),
				.component_id = static_cast<ecsact_component_like_id>(-1),
			};
		}

		std::vector<std::vector<ecsact_system_like_id>> batches;
		auto err = calculate_execution_batches(all_execution_order, batches);
		if(static_cast<int32_t>(err.system_id) == -1) {
			pkg.execution_batches = std::move(batches);
		}
		return err;
	}

	return {
		.system_id = static_cast<ecsact_system_like_id>(-1),
		.conflicting_system_id = static_cast<ecsact_system_like_id>(-1),
		.component_id = static_cast<ecsact_component_like_id>(-1),
	};
}

ecsact_execution_batches_error ecsact_check_system_execution_batches(
	ecsact_system_like_id system_id
) {
	auto& sys_def = get_system_like(system_id);
	if(sys_def.execution_order.empty()) {
		return {
			.system_id = static_cast<ecsact_system_like_id>(-1),
			.conflicting_system_id = static_cast<ecsact_system_like_id>(-1),
			.component_id = static_cast<ecsact_component_like_id>(-1),
		};
	}

	std::vector<std::vector<ecsact_system_like_id>> batches;
	auto err = calculate_execution_batches(sys_def.execution_order, batches);
	if(static_cast<int32_t>(err.system_id) == -1) {
		sys_def.execution_batches = std::move(batches);
	}
	return err;
}

bool ecsact_meta_is_system(ecsact_system_like_id system_id) {
	return sys_defs.contains(static_cast<ecsact_system_id>(system_id));
}

bool ecsact_meta_is_action(ecsact_system_like_id system_id) {
	return act_defs.contains(static_cast<ecsact_action_id>(system_id));
}

int32_t ecsact_meta_count_top_level_systems(ecsact_package_id package_id) {
	auto&   pkg_def = get_package_def(package_id);
	int32_t count = 0;
	for(auto& entry : pkg_def.execution_order) {
		if(std::holds_alternative<ecsact_system_like_id>(entry)) {
			count += 1;
		}
	}
	return count;
}

void ecsact_meta_get_top_level_systems(
	ecsact_package_id      package_id,
	int32_t                max_systems_count,
	ecsact_system_like_id* out_systems,
	int32_t*               out_systems_count
) {
	auto&   pkg_def = get_package_def(package_id);
	int32_t count = 0;
	for(auto& entry : pkg_def.execution_order) {
		if(count >= max_systems_count) {
			break;
		}
		if(auto sys_id = std::get_if<ecsact_system_like_id>(&entry)) {
			out_systems[count++] = *sys_id;
		}
	}

	if(out_systems_count != nullptr) {
		*out_systems_count = count;
	}
}

int32_t ecsact_meta_count_child_systems(ecsact_system_like_id system_id) {
	auto&   def = get_system_like(system_id);
	int32_t count = 0;
	for(auto& entry : def.execution_order) {
		if(auto inner_sys_id = std::get_if<ecsact_system_like_id>(&entry)) {
			if(ecsact_meta_is_system(*inner_sys_id)) {
				count += 1;
			}
		}
	}
	return count;
}

void ecsact_meta_get_child_system_ids(
	ecsact_system_like_id system_id,
	int32_t               max_child_system_ids_count,
	ecsact_system_id*     out_child_system_ids,
	int32_t*              out_child_system_count
) {
	auto&   def = get_system_like(system_id);
	int32_t count = 0;
	for(auto& entry : def.execution_order) {
		if(count >= max_child_system_ids_count) {
			break;
		}
		if(auto inner_sys_id = std::get_if<ecsact_system_like_id>(&entry)) {
			if(ecsact_meta_is_system(*inner_sys_id)) {
				out_child_system_ids[count++] =
					static_cast<ecsact_system_id>(*inner_sys_id);
			}
		}
	}

	if(out_child_system_count) {
		*out_child_system_count = count;
	}
}

ecsact_system_like_id ecsact_meta_get_parent_system_id(
	ecsact_system_id child_system_id
) {
	auto& def =
		get_system_like(ecsact_id_cast<ecsact_system_like_id>(child_system_id));
	return def.parent_system_id;
}

void ecsact_set_system_lazy_iteration_rate(
	ecsact_system_id system_id,
	int32_t          iteration_rate
) {
	auto& def = sys_defs.at(system_id);
	def.lazy_iteration_rate = iteration_rate;
}

int32_t ecsact_meta_get_lazy_iteration_rate(ecsact_system_id system_id) {
	auto& def = sys_defs.at(system_id);
	return def.lazy_iteration_rate;
}

ecsact_system_generates_id ecsact_add_system_generates(
	ecsact_system_like_id system_id
) {
	auto& sys_like_def = get_system_like(system_id);
	auto  gen_id = next_id<ecsact_system_generates_id>();
	sys_like_def.generates[gen_id] = {};
	return gen_id;
}

void ecsact_remove_system_generates(
	ecsact_system_like_id      system_id,
	ecsact_system_generates_id generates_id
) {
	auto& sys_like_def = get_system_like(system_id);
	sys_like_def.generates.erase(generates_id);
}

void ecsact_system_generates_set_component(
	ecsact_system_like_id      system_id,
	ecsact_system_generates_id generates_id,
	ecsact_component_id        component_id,
	ecsact_system_generate     generate_flag
) {
	auto& sys_like_def = get_system_like(system_id);
	sys_like_def.generates.at(generates_id)[component_id] = generate_flag;
}

void ecsact_system_generates_unset_component(
	ecsact_system_like_id      system_id,
	ecsact_system_generates_id generates_id,
	ecsact_component_id        component_id
) {
	auto& sys_like_def = get_system_like(system_id);
	sys_like_def.generates.at(generates_id).erase(component_id);
}

int32_t ecsact_meta_count_system_generates_ids(
	ecsact_system_like_id system_id
) {
	auto& sys_like_def = get_system_like(system_id);
	return static_cast<int32_t>(sys_like_def.generates.size());
}

void ecsact_meta_system_generates_ids(
	ecsact_system_like_id       system_id,
	int32_t                     max_generates_ids_count,
	ecsact_system_generates_id* out_generates_ids,
	int32_t*                    out_generates_ids_count
) {
	auto& def = get_system_like(system_id);
	auto  itr = def.generates.begin();
	for(int i = 0; max_generates_ids_count > i && itr != def.generates.end();
			++i) {
		out_generates_ids[i] = itr->first;
		++itr;
	}

	if(out_generates_ids_count) {
		*out_generates_ids_count = static_cast<int32_t>(def.generates.size());
	}
}

int32_t ecsact_meta_count_system_generates_components(
	ecsact_system_like_id      system_id,
	ecsact_system_generates_id generates_id
) {
	auto& def = get_system_like(system_id);
	return static_cast<int32_t>(def.generates.at(generates_id).size());
}

void ecsact_meta_system_generates_components(
	ecsact_system_like_id      system_id,
	ecsact_system_generates_id generates_id,
	int32_t                    max_components_count,
	ecsact_component_id*       out_component_ids,
	ecsact_system_generate*    out_generate_flags,
	int32_t*                   out_components_count
) {
	auto& def = get_system_like(system_id);
	auto& gens = def.generates.at(generates_id);
	auto  itr = gens.begin();
	for(int i = 0; max_components_count > i && itr != gens.end(); ++i) {
		out_component_ids[i] = itr->first;
		out_generate_flags[i] = itr->second;
		++itr;
	}

	if(out_components_count) {
		*out_components_count = static_cast<int32_t>(gens.size());
	}
}

void ecsact_set_system_parallel_execution(
	ecsact_system_like_id     system_id,
	ecsact_parallel_execution parallel_execution
) {
	auto& def = get_system_like(system_id);
	def.parallel_execution = parallel_execution;
}

ecsact_parallel_execution ecsact_meta_get_system_parallel_execution(
	ecsact_system_like_id system_id
) {
	auto& def = get_system_like(system_id);
	return def.parallel_execution;
}

void ecsact_set_system_notify_component_setting(
	ecsact_system_like_id        system_id,
	ecsact_component_like_id     component_id,
	ecsact_system_notify_setting setting
) {
	auto& def = get_system_like(system_id);
	if(setting == ECSACT_SYS_NOTIFY_NONE) {
		def.notify_settings.erase(component_id);
	} else {
		def.notify_settings[component_id] = setting;
	}
}

int32_t ecsact_meta_system_notify_settings_count(
	ecsact_system_like_id system_id
) {
	auto& def = get_system_like(system_id);
	return static_cast<int32_t>(def.notify_settings.size());
}

void ecsact_meta_system_notify_settings(
	ecsact_system_like_id         system_id,
	int32_t                       max_settings_count,
	ecsact_component_like_id*     out_component_ids,
	ecsact_system_notify_setting* out_notify_settings,
	int32_t*                      out_settings_count
) {
	auto& def = get_system_like(system_id);
	auto  itr = def.notify_settings.begin();
	for(int i = 0; max_settings_count > i && itr != def.notify_settings.end();
			++i) {
		out_component_ids[i] = itr->first;
		out_notify_settings[i] = itr->second;
		++itr;
	}

	if(out_settings_count) {
		*out_settings_count = static_cast<int32_t>(def.notify_settings.size());
	}
}

const char* ecsact_meta_cluster_name(ecsact_cluster_id cluster_id) {
	auto itr = cluster_defs.find(cluster_id);
	if(itr != cluster_defs.end()) {
		return itr->second.name.c_str();
	}
	return nullptr;
}

int32_t ecsact_meta_count_cluster_systems(ecsact_cluster_id cluster_id) {
	auto itr = cluster_defs.find(cluster_id);
	if(itr != cluster_defs.end()) {
		return static_cast<int32_t>(itr->second.systems.size());
	}
	return 0;
}

void ecsact_meta_get_cluster_systems(
	ecsact_cluster_id      cluster_id,
	int32_t                max_systems_count,
	ecsact_system_like_id* out_systems,
	int32_t*               out_systems_count
) {
	auto itr = cluster_defs.find(cluster_id);
	if(itr == cluster_defs.end()) {
		if(out_systems_count != nullptr) {
			*out_systems_count = 0;
		}
		return;
	}

	auto& def = itr->second;
	auto  count =
		std::min(max_systems_count, static_cast<int32_t>(def.systems.size()));

	for(int i = 0; count > i; ++i) {
		out_systems[i] = def.systems[i];
	}

	if(out_systems_count != nullptr) {
		*out_systems_count = static_cast<int32_t>(def.systems.size());
	}
}

ecsact_cluster_id ecsact_create_cluster(
	ecsact_package_id package_id,
	const char*       cluster_name,
	int32_t           cluster_name_len
) {
	auto pkg_itr = package_defs.find(package_id);
	if(pkg_itr == package_defs.end()) {
		return (ecsact_cluster_id)-1;
	}
	auto& pkg = pkg_itr->second;
	auto  id = next_id<ecsact_cluster_id>();
	auto& def = cluster_defs[id];
	def.name = std::string(cluster_name, cluster_name_len);
	pkg.execution_order.push_back(id);
	set_package_owner(ecsact_id_cast<ecsact_decl_id>(id), package_id);
	return id;
}

ecsact_cluster_id ecsact_create_system_cluster(
	ecsact_system_like_id parent_system_id,
	const char*           cluster_name,
	int32_t               cluster_name_len
) {
	auto& sys = get_system_like(parent_system_id);
	auto  id = next_id<ecsact_cluster_id>();
	auto& def = cluster_defs[id];
	def.name = std::string(cluster_name, cluster_name_len);
	sys.execution_order.push_back(id);
	set_package_owner(
		ecsact_id_cast<ecsact_decl_id>(id),
		owner_package_id(parent_system_id)
	);
	return id;
}

void ecsact_add_system_to_cluster(
	ecsact_cluster_id     cluster_id,
	ecsact_system_like_id system_id
) {
	auto def_itr = cluster_defs.find(cluster_id);
	if(def_itr == cluster_defs.end()) {
		return;
	}
	auto& def = def_itr->second;
	auto  itr = std::find(def.systems.begin(), def.systems.end(), system_id);
	if(itr != def.systems.end()) {
		return;
	}
	def.systems.push_back(system_id);

	auto owner_pkg_id = owner_package_id(system_id);
	if(owner_pkg_id == (ecsact_package_id)-1) {
		return;
	}

	auto parent_sys_id =
		ecsact_meta_get_parent_system_id(static_cast<ecsact_system_id>(system_id));

	auto& execution_order = (int32_t)parent_sys_id == -1
		? package_defs.at(owner_pkg_id).execution_order
		: get_system_like(parent_sys_id).execution_order;

	for(auto i = 0; execution_order.size() > i; ++i) {
		auto sys_id_ptr = std::get_if<ecsact_system_like_id>(&execution_order[i]);
		if(sys_id_ptr && *sys_id_ptr == system_id) {
			execution_order.erase(execution_order.begin() + i);
			--i;
		}
	}

	auto cluster_entry_itr = std::find_if(
		execution_order.begin(),
		execution_order.end(),
		[&](const auto& entry) {
			auto existing_cluster_id = std::get_if<ecsact_cluster_id>(&entry);
			return existing_cluster_id && *existing_cluster_id == cluster_id;
		}
	);

	if(cluster_entry_itr == execution_order.end()) {
		execution_order.push_back(cluster_id);
	}
}

auto ecsact_set_component_type( //
	ecsact_component_id   component_id,
	ecsact_component_type comp_type
) -> void {
	auto comp_def = comp_defs.find(component_id);
	if(comp_def == comp_defs.end()) {
		return;
	}

	comp_def->second.comp_type = comp_type;
}

ecsact_component_type ecsact_meta_component_type(
	ecsact_component_like_id comp_like_id
) {
	auto comp_id = static_cast<ecsact_component_id>(comp_like_id);
	if(!comp_defs.contains(comp_id)) {
		return ECSACT_COMPONENT_TYPE_NONE;
	}
	return comp_defs.at(comp_id).comp_type;
}
