#include "ecsact/interpret/eval.h"

#include <set>
#include <unordered_map>
#include <string_view>
#include <vector>
#include <string>
#include <string_view>
#include <cassert>
#include <optional>
#include <span>
#include <variant>
#include "ecsact/parse/status.h"
#include "ecsact/runtime/dynamic.h"
#include "ecsact/runtime/meta.hh"
#include "ecsact/runtime/meta.h"
#include "ecsact/interpret/detail/file_eval_error.hh"
#include "ecsact/interpret/eval_error.h"

using ecsact::meta::system_assoc_capabilities;
using ecsact::meta::system_capabilities;

using namespace std::string_literals;
using namespace std::string_view_literals;

// variant overloaded visit trick
// https://en.cppreference.com/w/cpp/utility/variant/visit
template<class... Ts>
struct overloaded : Ts... {
	using Ts::operator()...;
};
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

auto as_sv(ecsact_statement_sv sv) -> std::string_view {
	return std::string_view{sv.data, static_cast<size_t>(sv.length)};
}

static auto expect_context(
	std::span<const ecsact_statement>& context_stack,
	std::vector<ecsact_statement_type> context_types
) -> std::tuple<const ecsact_statement*, ecsact_eval_error> {
	if(context_stack.empty()) {
		for(auto context_type : context_types) {
			if(context_type == ECSACT_STATEMENT_NONE) {
				return {nullptr, ecsact_eval_error{}};
			}
		}
	}

	if(context_stack.empty()) {
		return {
			nullptr,
			ecsact_eval_error{
				.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
				.relevant_content = {},
				.context_type = ECSACT_STATEMENT_NONE,
			},
		};
	}

	auto& context = context_stack.back();
	for(auto context_type : context_types) {
		if(context.type == context_type) {
			return {&context, ecsact_eval_error{}};
		}
	}

	return {
		&context,
		ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
			.context_type = context.type,
		},
	};
}

static auto view_statement_params( //
	const ecsact_statement& statement
) -> std::span<const ecsact_statement_parameter> {
	return std::span{
		statement.parameters,
		static_cast<size_t>(statement.parameters_length),
	};
}

template<typename T>
static auto statement_param( //
	const ecsact_statement& statement,
	std::string_view        param_name
) -> std::optional<T>;

template<>
auto statement_param<int32_t>( //
	const ecsact_statement& statement,
	std::string_view        in_param_name
) -> std::optional<int32_t> {
	auto result = std::optional<int32_t>{};
	for(auto& param : view_statement_params(statement)) {
		auto param_name =
			std::string_view{param.name.data, static_cast<size_t>(param.name.length)};
		if(param_name != in_param_name) {
			continue;
		}

		if(param.value.type != ECSACT_STATEMENT_PARAM_VALUE_TYPE_INTEGER) {
			break;
		}

		result = param.value.data.integer_value;
	}

	return result;
}

template<>
auto statement_param<bool>( //
	const ecsact_statement& statement,
	std::string_view        param_name
) -> std::optional<bool> {
	auto result = std::optional<bool>{};
	for(auto& param : view_statement_params(statement)) {
		if(std::string_view{
				 param.name.data,
				 static_cast<size_t>(param.name.length)
			 } != param_name) {
			continue;
		}

		if(param.value.type != ECSACT_STATEMENT_PARAM_VALUE_TYPE_BOOL) {
			break;
		}

		result = param.value.data.bool_value;
	}

	return result;
}

template<>
auto statement_param<std::string_view>( //
	const ecsact_statement& statement,
	std::string_view        param_name
) -> std::optional<std::string_view> {
	auto result = std::optional<std::string_view>{};
	for(auto& param : view_statement_params(statement)) {
		if(std::string_view{
				 param.name.data,
				 static_cast<size_t>(param.name.length)
			 } != param_name) {
			continue;
		}

		if(param.value.type != ECSACT_STATEMENT_PARAM_VALUE_TYPE_STRING) {
			break;
		}

		result = std::string_view{
			param.value.data.string_value.data,
			static_cast<size_t>(param.value.data.string_value.length),
		};
	}

	return result;
}

template<typename FirstT, typename SecondT>
auto statement_param( //
	const ecsact_statement& statement,
	std::string_view        param_name
) -> std::optional<std::variant<FirstT, SecondT>> {
	using result_variant_t = std::variant<FirstT, SecondT>;

	{
		auto first_result = statement_param<FirstT>(statement, param_name);
		if(first_result) {
			return result_variant_t{*first_result};
		}
	}

	{
		auto second_result = statement_param<SecondT>(statement, param_name);
		if(second_result) {
			return result_variant_t{*second_result};
		}
	}

	return std::nullopt;
}

static auto parallel_param(const ecsact_statement& statement
) -> std::variant<ecsact_parallel_execution, ecsact_eval_error_code> {
	using result_t =
		std::variant<ecsact_parallel_execution, ecsact_eval_error_code>;

	auto param = statement_param<bool, std::string_view>(statement, "parallel");
	if(!param) {
		return ECSACT_PAR_EXEC_AUTO;
	}

	auto result = std::visit(
		overloaded{
			[&](bool param) -> result_t {
				return param ? ECSACT_PAR_EXEC_PREFERRED : ECSACT_PAR_EXEC_DENY;
			},
			[&](std::string_view param) -> result_t {
				if(param == "auto") {
					return ECSACT_PAR_EXEC_AUTO;
				} else if(param == "preferred") {
					return ECSACT_PAR_EXEC_PREFERRED;
				} else if(param == "deny") {
					return ECSACT_PAR_EXEC_DENY;
				} else {
					return ECSACT_EVAL_ERR_INVALID_PARAMETER_VALUE;
				}
			}
		},
		*param
	);
	return result;
}

auto allow_statement_params( //
	const ecsact_statement& statement,
	const ecsact_statement* context,
	auto&&                  allowed_param_names
) -> std::optional<ecsact_eval_error> {
	if(std::size(allowed_param_names) == 0 && statement.parameters_length > 0) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_PARAMETERS_NOT_ALLOWED,
			.relevant_content = {},
			.context_type = context ? context->type : ECSACT_STATEMENT_NONE,
		};
	}

	for(auto& param : view_statement_params(statement)) {
		auto param_name =
			std::string_view{param.name.data, static_cast<size_t>(param.name.length)};
		auto valid_param_name = false;

		for(auto&& allowed_param_name : allowed_param_names) {
			if(param_name == allowed_param_name) {
				valid_param_name = true;
				break;
			}
		}

		if(!valid_param_name) {
			return ecsact_eval_error{
				.code = ECSACT_EVAL_ERR_UNKNOWN_PARAMETER_NAME,
				.relevant_content = param.name,
				.context_type = context ? context->type : ECSACT_STATEMENT_NONE,
			};
		}
	}

	return std::nullopt;
}

auto disallow_statement_params( //
	const ecsact_statement& statement,
	const ecsact_statement* context
) -> std::optional<ecsact_eval_error> {
	return allow_statement_params(
		statement,
		context,
		std::array<std::string_view, 0>{}
	);
}

std::optional<ecsact_field_id> find_field_by_name(
	ecsact_composite_id compo_id,
	std::string_view    target_field_name
) {
	std::vector<ecsact_field_id> field_ids;
	field_ids.resize(ecsact_meta_count_fields(compo_id));
	ecsact_meta_get_field_ids(
		compo_id,
		static_cast<int32_t>(field_ids.size()),
		field_ids.data(),
		nullptr
	);
	for(auto& field_id : field_ids) {
		std::string field_name = ecsact_meta_field_name(compo_id, field_id);
		if(field_name == target_field_name) {
			return field_id;
		}
	}

	return {};
}

template<typename R, typename T>
static std::optional<R> cast_optional_id(std::optional<T> opt_id) {
	if(opt_id) {
		return ecsact_id_cast<R>(*opt_id);
	}

	return {};
}

template<typename T>
std::optional<T> find_by_name(
	ecsact_package_id  package_id,
	const std::string& name
);

template<typename T>
std::optional<T> find_by_statement(
	ecsact_package_id       package_id,
	const ecsact_statement& statement
);

template<>
std::optional<ecsact_component_id> find_by_name(
	ecsact_package_id  package_id,
	const std::string& lookup_name
) {
	auto pkg_name = ecsact::meta::package_name(package_id);
	for(auto id : ecsact::meta::get_component_ids(package_id)) {
		auto comp_name = ecsact::meta::component_name(id);
		if(lookup_name == comp_name) {
			return id;
		}

		if(lookup_name == pkg_name + "." + comp_name) {
			return id;
		}
	}

	for(auto dep_pkg_id : ecsact::meta::get_dependencies(package_id)) {
		auto dep_pkg_name = ecsact::meta::package_name(dep_pkg_id);

		for(auto id : ecsact::meta::get_component_ids(dep_pkg_id)) {
			auto comp_name = ecsact::meta::component_name(id);

			if(lookup_name == dep_pkg_name + "." + comp_name) {
				return id;
			}
		}
	}

	return {};
}

template<>
std::optional<ecsact_transient_id> find_by_name(
	ecsact_package_id  package_id,
	const std::string& lookup_name
) {
	auto pkg_name = ecsact::meta::package_name(package_id);
	for(auto id : ecsact::meta::get_transient_ids(package_id)) {
		auto trans_name = ecsact::meta::transient_name(id);
		if(lookup_name == trans_name) {
			return id;
		}

		if(lookup_name == pkg_name + "." + trans_name) {
			return id;
		}
	}

	for(auto dep_pkg_id : ecsact::meta::get_dependencies(package_id)) {
		auto dep_pkg_name = ecsact::meta::package_name(dep_pkg_id);

		for(auto id : ecsact::meta::get_transient_ids(dep_pkg_id)) {
			auto trans_name = ecsact::meta::transient_name(id);

			if(lookup_name == dep_pkg_name + "." + trans_name) {
				return id;
			}
		}
	}

	return {};
}

template<>
std::optional<ecsact_system_id> find_by_name(
	ecsact_package_id  package_id,
	const std::string& name
) {
	for(auto id : ecsact::meta::get_system_ids(package_id)) {
		if(name == ecsact::meta::system_name(id)) {
			return id;
		}
	}

	return {};
}

template<>
std::optional<ecsact_action_id> find_by_name(
	ecsact_package_id  package_id,
	const std::string& name
) {
	for(auto id : ecsact::meta::get_action_ids(package_id)) {
		if(name == ecsact::meta::action_name(id)) {
			return id;
		}
	}

	return {};
}

template<>
std::optional<ecsact_enum_id> find_by_name(
	ecsact_package_id  package_id,
	const std::string& lookup_name
) {
	auto pkg_name = ecsact::meta::package_name(package_id);
	for(auto id : ecsact::meta::get_enum_ids(package_id)) {
		auto enum_name = ecsact::meta::enum_name(id);
		if(lookup_name == enum_name) {
			return id;
		}

		if(lookup_name == pkg_name + "." + enum_name) {
			return id;
		}
	}

	for(auto dep_pkg_id : ecsact::meta::get_dependencies(package_id)) {
		auto dep_pkg_name = ecsact::meta::package_name(dep_pkg_id);

		for(auto id : ecsact::meta::get_enum_ids(dep_pkg_id)) {
			auto enum_name = ecsact::meta::enum_name(id);

			if(lookup_name == dep_pkg_name + "." + enum_name) {
				return id;
			}
		}
	}

	return {};
}

template<>
std::optional<ecsact_composite_id> find_by_name(
	ecsact_package_id  pkg_id,
	const std::string& name
) {
	if(auto id = find_by_name<ecsact_component_id>(pkg_id, name)) {
		return ecsact_id_cast<ecsact_composite_id>(*id);
	}

	if(auto id = find_by_name<ecsact_transient_id>(pkg_id, name)) {
		return ecsact_id_cast<ecsact_composite_id>(*id);
	}

	if(auto id = find_by_name<ecsact_action_id>(pkg_id, name)) {
		return ecsact_id_cast<ecsact_composite_id>(*id);
	}

	return {};
}

template<>
std::optional<ecsact_decl_id> find_by_name(
	ecsact_package_id  pkg_id,
	const std::string& name
) {
	if(auto id = find_by_name<ecsact_component_id>(pkg_id, name)) {
		return ecsact_id_cast<ecsact_decl_id>(*id);
	}

	if(auto id = find_by_name<ecsact_transient_id>(pkg_id, name)) {
		return ecsact_id_cast<ecsact_decl_id>(*id);
	}

	if(auto id = find_by_name<ecsact_system_id>(pkg_id, name)) {
		return ecsact_id_cast<ecsact_decl_id>(*id);
	}

	if(auto id = find_by_name<ecsact_action_id>(pkg_id, name)) {
		return ecsact_id_cast<ecsact_decl_id>(*id);
	}

	return {};
}

template<>
std::optional<ecsact_component_like_id> find_by_name(
	ecsact_package_id  pkg_id,
	const std::string& name
) {
	if(auto id = find_by_name<ecsact_component_id>(pkg_id, name)) {
		return ecsact_id_cast<ecsact_component_like_id>(*id);
	}

	if(auto id = find_by_name<ecsact_transient_id>(pkg_id, name)) {
		return ecsact_id_cast<ecsact_component_like_id>(*id);
	}

	return {};
}

auto find_user_field_type_by_name(
	ecsact_package_id package_id,
	std::string_view  user_type_name,
	int32_t           length
) -> std::optional<ecsact_field_type> {
	auto enum_id =
		find_by_name<ecsact_enum_id>(package_id, std::string{user_type_name});
	if(enum_id) {
		return ecsact_field_type{
			.kind = ECSACT_TYPE_KIND_ENUM,
			.type{.enum_id = *enum_id},
			.length = length,
		};
	}

	return {};
}

auto find_field_by_full_name(
	ecsact_package_id package_id,
	std::string_view  field_full_name
) -> std::optional<ecsact_field_type> {
	auto last_dot_idx = field_full_name.find_last_of('.');
	if(last_dot_idx == std::string_view::npos) {
		return {};
	}

	auto composite_name = field_full_name.substr(0, last_dot_idx);
	auto field_name = field_full_name.substr(last_dot_idx + 1);

	auto composite_id =
		find_by_name<ecsact_composite_id>(package_id, std::string{composite_name});

	if(!composite_id) {
		return {};
	}

	auto field_id = find_field_by_name(*composite_id, field_name);

	if(!field_id) {
		return {};
	}

	return ecsact_field_type{
		.kind = ECSACT_TYPE_KIND_FIELD_INDEX,
		.type{
			.field_index{
				.composite_id = *composite_id,
				.field_id = *field_id,
			},
		},
	};
}

template<>
std::optional<ecsact_composite_id> find_by_statement(
	ecsact_package_id       package_id,
	const ecsact_statement& statement
) {
	switch(statement.type) {
		case ECSACT_STATEMENT_COMPONENT:
			return cast_optional_id<ecsact_composite_id>(
				find_by_name<ecsact_component_id>(
					package_id,
					std::string(
						statement.data.component_statement.component_name.data,
						statement.data.component_statement.component_name.length
					)
				)
			);
		case ECSACT_STATEMENT_TRANSIENT:
			return cast_optional_id<ecsact_composite_id>(
				find_by_name<ecsact_transient_id>(
					package_id,
					std::string(
						statement.data.transient_statement.transient_name.data,
						statement.data.transient_statement.transient_name.length
					)
				)
			);
		case ECSACT_STATEMENT_ACTION:
			return cast_optional_id<ecsact_composite_id>(
				find_by_name<ecsact_action_id>(
					package_id,
					std::string(
						statement.data.action_statement.action_name.data,
						statement.data.action_statement.action_name.length
					)
				)
			);
		default:
			break;
	}

	return {};
}

template<>
std::optional<ecsact_component_like_id> find_by_statement(
	ecsact_package_id       package_id,
	const ecsact_statement& statement
) {
	switch(statement.type) {
		case ECSACT_STATEMENT_COMPONENT:
			return cast_optional_id<ecsact_component_like_id>(
				find_by_name<ecsact_component_id>(
					package_id,
					std::string(
						statement.data.component_statement.component_name.data,
						statement.data.component_statement.component_name.length
					)
				)
			);
		case ECSACT_STATEMENT_TRANSIENT:
			return cast_optional_id<ecsact_component_like_id>(
				find_by_name<ecsact_transient_id>(
					package_id,
					std::string(
						statement.data.transient_statement.transient_name.data,
						statement.data.transient_statement.transient_name.length
					)
				)
			);
		case ECSACT_STATEMENT_SYSTEM_COMPONENT:
			return find_by_name<ecsact_component_like_id>(
				package_id,
				std::string(
					statement.data.system_component_statement.component_name.data,
					statement.data.system_component_statement.component_name.length
				)
			);
		default:
			break;
	}

	return {};
}

template<>
std::optional<ecsact_system_like_id> find_by_statement(
	ecsact_package_id       package_id,
	const ecsact_statement& statement
) {
	switch(statement.type) {
		case ECSACT_STATEMENT_SYSTEM:
			return cast_optional_id<ecsact_system_like_id>(
				find_by_name<ecsact_system_id>(
					package_id,
					std::string(
						statement.data.system_statement.system_name.data,
						statement.data.system_statement.system_name.length
					)
				)
			);
		case ECSACT_STATEMENT_ACTION:
			return cast_optional_id<ecsact_system_like_id>(
				find_by_name<ecsact_action_id>(
					package_id,
					std::string(
						statement.data.action_statement.action_name.data,
						statement.data.action_statement.action_name.length
					)
				)
			);
		default:
			break;
	}

	return {};
}

static ecsact_eval_error
eval_none_statement(ecsact_package_id, std::span<const ecsact_statement>&, const ecsact_statement&) {
	return {};
}

static ecsact_eval_error
eval_unknown_statement(ecsact_package_id, std::span<const ecsact_statement>&, const ecsact_statement&) {
	return {};
}

static ecsact_eval_error eval_import_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) {
	auto& data = statement.data.import_statement;
	auto [context, err] = expect_context(context_stack, {ECSACT_STATEMENT_NONE});
	if(err.code != ECSACT_EVAL_OK) {
		err.relevant_content = data.import_package_name;
		return err;
	}

	if(auto err = disallow_statement_params(statement, context)) {
		return *err;
	}

	auto import_name = std::string{
		data.import_package_name.data,
		static_cast<size_t>(data.import_package_name.length),
	};

	for(auto dep_pkg_id : ecsact::meta::get_package_ids()) {
		if(dep_pkg_id == package_id) {
			continue;
		}
		if(ecsact::meta::package_name(dep_pkg_id) == import_name) {
			ecsact_add_dependency(package_id, dep_pkg_id);
			return {};
		}
	}

	return ecsact_eval_error{
		.code = ECSACT_EVAL_ERR_UNKNOWN_IMPORT,
		.relevant_content = data.import_package_name,
	};
}

static ecsact_eval_error eval_component_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) {
	auto& data = statement.data.component_statement;
	auto [context, err] = expect_context(context_stack, {ECSACT_STATEMENT_NONE});
	if(err.code != ECSACT_EVAL_OK) {
		err.relevant_content = data.component_name;
		return err;
	}

	constexpr auto allowed_params = std::array{"stream"sv, "transient"sv};
	if(auto err = allow_statement_params(statement, context, allowed_params)) {
		return *err;
	}

	auto stream_param =
		statement_param<bool, std::string_view>(statement, "stream"sv);
	auto transient_param = statement_param<bool>(statement, "transient"sv);
	auto component_type = ECSACT_COMPONENT_TYPE_NONE;

	if(stream_param) {
		auto stream_type = std::get_if<std::string_view>(&stream_param.value());
		if(stream_type) {
			if(*stream_type != "lazy"sv) {
				return ecsact_eval_error{
					.code = ECSACT_EVAL_ERR_INVALID_PARAMETER_VALUE,
					.relevant_content = statement.parameters[0].name,
				};
			}

			component_type = ECSACT_COMPONENT_TYPE_LAZY_STREAM;
		} else if(std::get<bool>(stream_param.value())) {
			component_type = ECSACT_COMPONENT_TYPE_STREAM;
		}
	}

	if(transient_param) {
		if(transient_param.value()) {
			if(component_type != ECSACT_COMPONENT_TYPE_NONE) {
				// can't have transient stream
				return ecsact_eval_error{
					.code = ECSACT_EVAL_ERR_INVALID_PARAMETER_VALUE,
					.relevant_content = statement.parameters[0].name,
				};
			}

			component_type = ECSACT_COMPONENT_TYPE_TRANSIENT;
		}
	}

	auto name = std::string(data.component_name.data, data.component_name.length);

	auto existing_decl = find_by_name<ecsact_decl_id>(package_id, name);
	if(existing_decl) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_DECLARATION_NAME_TAKEN,
			.relevant_content = data.component_name,
		};
	}

	auto comp_id = ecsact_create_component(
		package_id,
		data.component_name.data,
		data.component_name.length
	);

	ecsact_set_component_type(comp_id, component_type);

	return {};
}

static ecsact_eval_error eval_transient_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) {
	auto& data = statement.data.transient_statement;
	auto [context, err] = expect_context(context_stack, {ECSACT_STATEMENT_NONE});
	if(err.code != ECSACT_EVAL_OK) {
		err.relevant_content = data.transient_name;
		return err;
	}

	if(auto err = disallow_statement_params(statement, context)) {
		return *err;
	}

	auto name = std::string(data.transient_name.data, data.transient_name.length);
	auto existing_decl = find_by_name<ecsact_decl_id>(package_id, name);
	if(existing_decl) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_DECLARATION_NAME_TAKEN,
			.relevant_content = data.transient_name,
		};
	}

	ecsact_create_transient(
		package_id,
		data.transient_name.data,
		data.transient_name.length
	);

	return {};
}

static ecsact_eval_error eval_system_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) {
	auto& data = statement.data.system_statement;
	auto  parent_sys_like_id = std::optional<ecsact_system_like_id>{};
	auto [context, err] = expect_context(
		context_stack,
		{
			ECSACT_STATEMENT_NONE,
			ECSACT_STATEMENT_SYSTEM,
			ECSACT_STATEMENT_ACTION,
		}
	);

	if(err.code != ECSACT_EVAL_OK) {
		err.relevant_content = data.system_name;
		return err;
	}

	if(auto err = allow_statement_params(
			 statement,
			 context,
			 std::array{"lazy"sv, "parallel"sv}
		 )) {
		return *err;
	}

	auto lazy_value = [&]() -> int32_t {
		auto lazy_param = statement_param<bool, int32_t>(statement, "lazy");
		if(!lazy_param) {
			return 0;
		}

		return std::visit(
			overloaded{
				[](bool bool_lazy) -> int32_t { return bool_lazy ? 1 : 0; },
				[](int32_t int_lazy) -> int32_t { return int_lazy; },
			},
			*lazy_param
		);
	}();

	if(context != nullptr) {
		parent_sys_like_id =
			find_by_statement<ecsact_system_like_id>(package_id, *context);
		if(!parent_sys_like_id) {
			return ecsact_eval_error{
				.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
				.relevant_content = {},
			};
		}
	}

	auto name = std::string(data.system_name.data, data.system_name.length);

	auto existing_decl = find_by_name<ecsact_decl_id>(package_id, name);
	if(existing_decl) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_DECLARATION_NAME_TAKEN,
			.relevant_content = data.system_name,
		};
	}

	auto sys_id = ecsact_create_system(
		package_id,
		data.system_name.data,
		data.system_name.length
	);

	if(parent_sys_like_id) {
		ecsact_add_child_system(*parent_sys_like_id, sys_id);
	}

	if(lazy_value > 0) {
		ecsact_set_system_lazy_iteration_rate(sys_id, lazy_value);
	}

	auto parallel = parallel_param(statement);
	if(auto err_code = std::get_if<ecsact_eval_error_code>(&parallel)) {
		return ecsact_eval_error{
			.code = *err_code,
			.relevant_content = data.system_name,
		};
	}

	ecsact_set_system_parallel_execution(
		ecsact_id_cast<ecsact_system_like_id>(sys_id),
		std::get<ecsact_parallel_execution>(parallel)
	);

	return {};
}

static ecsact_eval_error eval_action_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) {
	auto& data = statement.data.action_statement;
	auto [context, err] = expect_context(context_stack, {ECSACT_STATEMENT_NONE});
	if(err.code != ECSACT_EVAL_OK) {
		err.relevant_content = data.action_name;
		return err;
	}

	if(auto err =
			 allow_statement_params(statement, context, std::array{"parallel"sv})) {
		return *err;
	}

	auto name = std::string(data.action_name.data, data.action_name.length);

	auto existing_decl = find_by_name<ecsact_decl_id>(package_id, name);
	if(existing_decl) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_DECLARATION_NAME_TAKEN,
			.relevant_content = data.action_name,
		};
	}

	auto act_id = ecsact_create_action(
		package_id,
		data.action_name.data,
		data.action_name.length
	);

	auto parallel = parallel_param(statement);
	if(auto err_code = std::get_if<ecsact_eval_error_code>(&parallel)) {
		return ecsact_eval_error{
			.code = *err_code,
			.relevant_content = data.action_name,
		};
	}

	ecsact_set_system_parallel_execution(
		ecsact_id_cast<ecsact_system_like_id>(act_id),
		std::get<ecsact_parallel_execution>(parallel)
	);

	return {};
}

static ecsact_eval_error eval_enum_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) {
	auto& data = statement.data.enum_statement;
	auto [context, err] = expect_context(context_stack, {ECSACT_STATEMENT_NONE});
	if(err.code != ECSACT_EVAL_OK) {
		err.relevant_content = data.enum_name;
		return err;
	}

	if(auto err = disallow_statement_params(statement, context)) {
		return *err;
	}

	auto name = std::string(data.enum_name.data, data.enum_name.length);

	auto existing_decl = find_by_name<ecsact_decl_id>(package_id, name);
	if(existing_decl) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_DECLARATION_NAME_TAKEN,
			.relevant_content = data.enum_name,
		};
	}

	ecsact_create_enum(package_id, data.enum_name.data, data.enum_name.length);

	return {};
}

static ecsact_eval_error eval_enum_value_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) {
	auto& data = statement.data.enum_value_statement;
	auto [context, err] = expect_context(context_stack, {ECSACT_STATEMENT_ENUM});
	if(err.code != ECSACT_EVAL_OK) {
		err.relevant_content = data.name;
		return err;
	}

	if(auto err = disallow_statement_params(statement, context)) {
		return *err;
	}

	auto& context_data = context->data.enum_statement;
	auto  enum_name =
		std::string(context_data.enum_name.data, context_data.enum_name.length);

	auto enum_id = find_by_name<ecsact_enum_id>(package_id, enum_name);
	if(!enum_id) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = context_data.enum_name,
		};
	}

	ecsact_add_enum_value(*enum_id, data.value, data.name.data, data.name.length);

	return {};
}

static ecsact_eval_error eval_builtin_type_field_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) {
	auto& data = statement.data.field_statement;
	auto [context, err] = expect_context(
		context_stack,
		{
			ECSACT_STATEMENT_COMPONENT,
			ECSACT_STATEMENT_TRANSIENT,
			ECSACT_STATEMENT_ACTION,
		}
	);
	if(err.code != ECSACT_EVAL_OK) {
		err.relevant_content = data.field_name;
		return err;
	}

	if(auto err = disallow_statement_params(statement, context)) {
		return *err;
	}

	auto compo_id = find_by_statement<ecsact_composite_id>(package_id, *context);
	if(!compo_id) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	std::string field_name(data.field_name.data, data.field_name.length);

	for(auto field_id : ecsact::meta::get_field_ids(*compo_id)) {
		std::string other_field_name = ecsact_meta_field_name(*compo_id, field_id);
		if(field_name == other_field_name) {
			return ecsact_eval_error{
				.code = ECSACT_EVAL_ERR_FIELD_NAME_ALREADY_EXISTS,
				.relevant_content = data.field_name,
			};
		}
	}

	ecsact_add_field(
		*compo_id,
		ecsact_field_type{
			.kind = ECSACT_TYPE_KIND_BUILTIN,
			.type{.builtin = data.field_type},
			.length = data.length,
		},
		data.field_name.data,
		data.field_name.length
	);

	return {};
}

static ecsact_eval_error eval_user_type_field_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) {
	auto& data = statement.data.user_type_field_statement;
	auto [context, err] = expect_context(
		context_stack,
		{
			ECSACT_STATEMENT_COMPONENT,
			ECSACT_STATEMENT_TRANSIENT,
			ECSACT_STATEMENT_ACTION,
		}
	);

	if(err.code != ECSACT_EVAL_OK) {
		err.relevant_content = data.user_type_name;
		return err;
	}

	if(auto err = disallow_statement_params(statement, context)) {
		return *err;
	}

	auto compo_id =
		find_by_statement<ecsact_composite_id>(package_id, context_stack.back());
	if(!compo_id) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	std::string field_name(data.field_name.data, data.field_name.length);

	for(auto field_id : ecsact::meta::get_field_ids(*compo_id)) {
		std::string other_field_name = ecsact_meta_field_name(*compo_id, field_id);
		if(field_name == other_field_name) {
			return ecsact_eval_error{
				.code = ECSACT_EVAL_ERR_FIELD_NAME_ALREADY_EXISTS,
				.relevant_content = data.field_name,
			};
		}
	}

	auto field_type_lookup =
		std::string_view(data.user_type_name.data, data.user_type_name.length);

	auto user_field_type =
		find_user_field_type_by_name(package_id, field_type_lookup, data.length);

	auto field_index_field_type =
		find_field_by_full_name(package_id, field_type_lookup);

	if(!user_field_type && !field_index_field_type) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_UNKNOWN_FIELD_TYPE,
			.relevant_content = data.user_type_name,
		};
	}

	if(user_field_type && field_index_field_type) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_AMBIGUOUS_FIELD_TYPE,
			.relevant_content = data.user_type_name,
		};
	}

	auto field_type = user_field_type //
		? *user_field_type
		: *field_index_field_type;

	ecsact_add_field(
		*compo_id,
		field_type,
		data.field_name.data,
		data.field_name.length
	);

	return {};
}

static ecsact_eval_error eval_entity_field_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) {
	return eval_builtin_type_field_statement(
		package_id,
		context_stack,
		statement
	);
}

static auto get_with_field_ids(
	ecsact_system_like_id                sys_like_id,
	ecsact_component_like_id             comp_like_id,
	std::span<const ecsact_statement_sv> fields
) -> std::vector<ecsact_field_id> {
	auto with_field_ids = std::vector<ecsact_field_id>{};
	for(int i = 0; fields.size() > i; ++i) {
		auto assoc_field_name =
			std::string_view{fields[i].data, static_cast<size_t>(fields[i].length)};

		auto assoc_field_id = std::optional<ecsact_field_id>{};

		for(auto field_id : ecsact::meta::get_field_ids(comp_like_id)) {
			std::string field_name = ecsact_meta_field_name(
				ecsact_id_cast<ecsact_composite_id>(comp_like_id),
				field_id
			);
			if(assoc_field_name == field_name) {
				assoc_field_id = field_id;
				break;
			}
		}

		assert(assoc_field_id.has_value());
		with_field_ids.emplace_back(*assoc_field_id);
	}
	return with_field_ids;
}

static auto find_capabilities_for( //
	auto sys_like_id,
	auto id
) -> std::optional<ecsact_system_capability> {
	for(auto&& [comp_id, caps] : ecsact::meta::system_capabilities(sys_like_id)) {
		if(ecsact_id_cast<decltype(id)>(comp_id) == id) {
			return caps;
		}
	}

	auto parent = ecsact::meta::get_parent_system_id(
		// NOTE: this cast only works because we wrote the meta runtime for the
		// interpreter. This is bad practice.
		static_cast<ecsact_system_id>(sys_like_id)
	);
	if(parent) {
		return find_capabilities_for(*parent, id);
	}

	return {};
}

static auto eval_system_with_statement_data_common(
	ecsact_system_like_id                sys_like_id,
	ecsact_component_like_id             comp_like_id,
	std::span<const ecsact_statement_sv> fields
) -> ecsact_eval_error {
	auto with_field_ids = std::vector<ecsact_field_id>{};
	for(int i = 0; fields.size() > i; ++i) {
		auto assoc_field_name =
			std::string_view{fields[i].data, static_cast<size_t>(fields[i].length)};

		auto assoc_field_id = std::optional<ecsact_field_id>{};

		for(auto field_id : ecsact::meta::get_field_ids(comp_like_id)) {
			std::string field_name = ecsact_meta_field_name(
				ecsact_id_cast<ecsact_composite_id>(comp_like_id),
				field_id
			);
			if(assoc_field_name == field_name) {
				assoc_field_id = field_id;
				break;
			}
		}

		if(!assoc_field_id) {
			return ecsact_eval_error{
				.code = ECSACT_EVAL_ERR_UNKNOWN_FIELD_NAME,
				.relevant_content = fields[i],
			};
		}

		auto field_type =
			ecsact::meta::get_field_type(comp_like_id, *assoc_field_id);

		if(field_type.kind == ECSACT_TYPE_KIND_BUILTIN) {
			if(field_type.type.builtin != ECSACT_ENTITY_TYPE) {
				return ecsact_eval_error{
					.code = ECSACT_EVAL_ERR_INVALID_ASSOC_FIELD_TYPE,
					.relevant_content = fields[i],
				};
			}
		} else if(field_type.kind != ECSACT_TYPE_KIND_FIELD_INDEX) {
			return ecsact_eval_error{
				.code = ECSACT_EVAL_ERR_INVALID_ASSOC_FIELD_TYPE,
				.relevant_content = fields[i],
			};
		}

		with_field_ids.emplace_back(*assoc_field_id);
	}

	if(with_field_ids.empty()) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_UNEXPECTED_STATEMENT,
			.relevant_content = {},
		};
	}

	auto assoc_id = ecsact_add_system_assoc(sys_like_id, comp_like_id);

	for(auto assoc_field_id : with_field_ids) {
		ecsact_add_system_assoc_field(sys_like_id, assoc_id, assoc_field_id);
	}

	return {};
}

template<typename SystemLikeID, typename ComponentLikeID>
static auto find_assoc_ids_with_fields( //
	SystemLikeID    sys_like_id,
	ComponentLikeID comp_like_id,
	auto            target_field_names
) -> std::vector<ecsact_system_assoc_id> {
	assert(std::size(target_field_names) > 0);
	auto assoc_ids = std::set<ecsact_system_assoc_id>{};
	for(auto assoc_id : ecsact::meta::system_assoc_ids(sys_like_id)) {
		auto assoc_comp_id =
			ecsact::meta::system_assoc_component_id(sys_like_id, assoc_id);
		if(assoc_comp_id != comp_like_id) {
			continue;
		}

		auto fields = ecsact::meta::system_assoc_fields(sys_like_id, assoc_id);
		auto matching_fields = std::vector<ecsact_field_id>{};
		for(auto field : fields) {
			auto field_name = ecsact::meta::field_name(comp_like_id, field);
			auto found_target_field = false;
			for(auto target_field_name : target_field_names) {
				if(field_name == as_sv(target_field_name)) {
					found_target_field = true;
					matching_fields.push_back(field);
				}
			}

			if(!found_target_field) {
				break;
			}
		}

		assert(matching_fields.size() <= fields.size());
		if(matching_fields.size() == fields.size()) {
			assoc_ids.insert(assoc_id);
		}
	}

	assert(!assoc_ids.empty());

	return std::vector<ecsact_system_assoc_id>{
		assoc_ids.begin(),
		assoc_ids.end()
	};
}

static ecsact_eval_error eval_system_component_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) {
	auto [context, err] = expect_context(
		context_stack,
		{
			ECSACT_STATEMENT_SYSTEM,
			ECSACT_STATEMENT_ACTION,
			ECSACT_STATEMENT_SYSTEM_COMPONENT,
			ECSACT_STATEMENT_SYSTEM_WITH,
		}
	);

	if(err.code != ECSACT_EVAL_OK) {
		err.relevant_content =
			statement.data.system_component_statement.component_name;
		return err;
	}

	if(auto err = disallow_statement_params(statement, context)) {
		return *err;
	}

	auto& statement_data = statement.data.system_component_statement;
	auto  sys_like_id = std::optional<ecsact_system_like_id>{};
	auto  assoc_id = std::optional<ecsact_system_assoc_id>{};

	std::string comp_like_name(
		statement_data.component_name.data,
		statement_data.component_name.length
	);

	auto comp_like_id =
		find_by_name<ecsact_component_like_id>(package_id, comp_like_name);

	if(!comp_like_id) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_UNKNOWN_COMPONENT_LIKE_TYPE,
			.relevant_content = statement_data.component_name,
		};
	}

	// See expect_context above for options here
	switch(context->type) {
		// system Example {
		//     readwrite ExampleComponent with blah  <-- we are here
		// }
		case ECSACT_STATEMENT_SYSTEM:
		case ECSACT_STATEMENT_ACTION: {
			sys_like_id =
				find_by_statement<ecsact_system_like_id>(package_id, *context);
			break;
		}
		// system Example {
		//     readwrite ExampleComponent with blah {
		//         readwrite ExampleComponent  <-- we are here
		//     }
		// }
		case ECSACT_STATEMENT_SYSTEM_COMPONENT: {
			if(context_stack.size() < 2) {
				return ecsact_eval_error{
					.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
					.relevant_content = {},
				};
			}
			sys_like_id = find_by_statement<ecsact_system_like_id>(
				package_id,
				context_stack[context_stack.size() - 2]
			);

			if(!sys_like_id) {
				return ecsact_eval_error{
					.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
					.relevant_content = {},
				};
			}

			if(statement_data.with_field_name_list_count > 0) {
				return ecsact_eval_error{
					.code = ECSACT_EVAL_ERR_NESTED_ASSOC,
					.relevant_content = statement_data.with_field_name_list[0],
				};
			}

			const auto& context_data = context->data.system_component_statement;

			auto assoc_comp_id =
				find_by_statement<ecsact_component_like_id>(package_id, *context);
			if(!assoc_comp_id) {
				return ecsact_eval_error{
					.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
					.relevant_content = {},
				};
			}

			auto field_names = std::span{
				std::data(context_data.with_field_name_list),
				static_cast<size_t>(context_data.with_field_name_list_count)
			};

			if(!field_names.empty()) {
				// NOTE: This is a temporary limitation of the interpreter since there
				// isn't a way to get the association ID other than comparing fields.
				auto assoc_ids =
					find_assoc_ids_with_fields(*sys_like_id, *assoc_comp_id, field_names);
				if(assoc_ids.size() > 1) {
					return ecsact_eval_error{
						.code = ECSACT_EVAL_ERR_SAME_FIELDS_SYSTEM_ASSOCIATION,
						.relevant_content = {},
					};
				}
				assoc_id = assoc_ids.at(0);
			}
			break;
		}
		// system Example {
		//     readwrite ExampleComponent {
		//        with blah {
		//            readwrite ExampleComponent <-- we are here
		//        }
		//     }
		// }
		case ECSACT_STATEMENT_SYSTEM_WITH: {
			if(context_stack.size() < 3) {
				return ecsact_eval_error{
					.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
					.relevant_content = {},
				};
			}
			sys_like_id = find_by_statement<ecsact_system_like_id>(
				package_id,
				context_stack[context_stack.size() - 3]
			);
			if(!sys_like_id) {
				return ecsact_eval_error{
					.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
					.relevant_content = {},
				};
			}

			const auto& context_data = context->data.system_with_statement;

			auto assoc_comp_id = find_by_statement<ecsact_component_like_id>(
				package_id,
				context_stack[context_stack.size() - 2]
			);
			if(!assoc_comp_id) {
				return ecsact_eval_error{
					.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
					.relevant_content = {},
				};
			}

			auto field_names = std::span{
				std::data(context_data.with_field_name_list),
				static_cast<size_t>(context_data.with_field_name_list_count)
			};

			// NOTE: This is a temporary limitation of the interpreter since there
			// isn't a way to get the association ID other than comparing fields.
			auto assoc_ids =
				find_assoc_ids_with_fields(*sys_like_id, *assoc_comp_id, field_names);
			if(assoc_ids.size() > 1) {
				return ecsact_eval_error{
					.code = ECSACT_EVAL_ERR_SAME_FIELDS_SYSTEM_ASSOCIATION,
					.relevant_content = {},
				};
			}
			assoc_id = assoc_ids.at(0);
			break;
		}
		default:
			return ecsact_eval_error{
				.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
				.relevant_content = {},
			};
	}

	if(!sys_like_id) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	if(ecsact::meta::system_notify_settings_count(*sys_like_id) > 0) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_NOTIFY_BEFORE_SYSTEM_COMPONENT,
			.relevant_content = {},
		};
	}

	if(statement_data.with_field_name_list_count > 0) {
		auto status = eval_system_with_statement_data_common(
			*sys_like_id,
			*comp_like_id,
			std::span{
				std::data(statement_data.with_field_name_list),
				static_cast<size_t>(statement_data.with_field_name_list_count)
			}
		);

		if(status.code != ECSACT_EVAL_OK) {
			return status;
		}
	}

	if(assoc_id) {
		for(auto& entry : system_assoc_capabilities(*sys_like_id, *assoc_id)) {
			if(entry.first == *comp_like_id) {
				return ecsact_eval_error{
					.code = ECSACT_EVAL_ERR_MULTIPLE_CAPABILITIES_SAME_COMPONENT_LIKE,
					.relevant_content = statement_data.component_name,
				};
			}
		}
	} else {
		for(auto& entry : system_capabilities(*sys_like_id)) {
			if(entry.first == *comp_like_id) {
				return ecsact_eval_error{
					.code = ECSACT_EVAL_ERR_MULTIPLE_CAPABILITIES_SAME_COMPONENT_LIKE,
					.relevant_content = statement_data.component_name,
				};
			}
		}
	}

	if(assoc_id) {
		ecsact_set_system_assoc_capability(
			*sys_like_id,
			*assoc_id,
			*comp_like_id,
			statement_data.capability
		);
	} else {
		ecsact_set_system_capability(
			*sys_like_id,
			*comp_like_id,
			statement_data.capability
		);
	}

	return {};
}

static ecsact_eval_error eval_system_generates_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) {
	auto [context, err] = expect_context(
		context_stack,
		{
			ECSACT_STATEMENT_SYSTEM,
			ECSACT_STATEMENT_ACTION,
		}
	);

	if(err.code != ECSACT_EVAL_OK) {
		return err;
	}

	if(auto err = disallow_statement_params(statement, context)) {
		return *err;
	}

	auto sys_like_id =
		find_by_statement<ecsact_system_like_id>(package_id, context_stack.back());

	if(!sys_like_id) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	auto gen_ids = ecsact::meta::get_system_generates_ids(*sys_like_id);
	if(!gen_ids.empty()) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_ONLY_ONE_GENERATES_BLOCK_ALLOWED,
			.relevant_content = {},
		};
	}

	ecsact_add_system_generates(*sys_like_id);

	return {};
}

static ecsact_eval_error eval_system_with_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) {
	if(context_stack.size() < 2) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	auto [context, err] =
		expect_context(context_stack, {ECSACT_STATEMENT_SYSTEM_COMPONENT});

	if(err.code != ECSACT_EVAL_OK) {
		if(statement.data.system_with_statement.with_field_name_list_count > 0) {
			err.relevant_content =
				statement.data.system_with_statement.with_field_name_list[0];
		}
		return err;
	}

	if(auto err = disallow_statement_params(statement, context)) {
		return *err;
	}

	auto& context_data = context->data.system_component_statement;
	auto& sys_like_statement = context_stack[context_stack.size() - 2];
	auto  sys_like_id =
		find_by_statement<ecsact_system_like_id>(package_id, sys_like_statement);

	if(!sys_like_id) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	auto comp_name = std::string(
		context_data.component_name.data,
		context_data.component_name.length
	);

	auto comp_like_id =
		find_by_name<ecsact_component_like_id>(package_id, comp_name);

	if(!comp_like_id) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_UNKNOWN_COMPONENT_LIKE_TYPE,
			.relevant_content = context_data.component_name,
		};
	}

	auto& data = statement.data.system_with_statement;

	return eval_system_with_statement_data_common(
		*sys_like_id,
		*comp_like_id,
		std::span{
			std::data(data.with_field_name_list),
			static_cast<size_t>(data.with_field_name_list_count)
		}
	);
}

static auto get_notify_setting_from_string( //
	std::string_view setting_name
) -> std::optional<ecsact_system_notify_setting> {
	static const auto notify_setting_name_map =
		std::unordered_map<std::string_view, ecsact_system_notify_setting>{
			{"always"sv, ECSACT_SYS_NOTIFY_ALWAYS},
			{"oninit"sv, ECSACT_SYS_NOTIFY_ONINIT},
			{"onupdate"sv, ECSACT_SYS_NOTIFY_ONUPDATE},
			{"onchange"sv, ECSACT_SYS_NOTIFY_ONCHANGE},
			{"onremove"sv, ECSACT_SYS_NOTIFY_ONREMOVE},
		};

	auto itr = notify_setting_name_map.find(setting_name);
	if(itr != notify_setting_name_map.end()) {
		return itr->second;
	}

	return std::nullopt;
}

static auto eval_system_notify_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) -> ecsact_eval_error {
	auto [context, err] = expect_context(
		context_stack,
		{
			ECSACT_STATEMENT_SYSTEM,
			ECSACT_STATEMENT_ACTION,
		}
	);

	if(err.code != ECSACT_EVAL_OK) {
		return err;
	}

	if(auto err = disallow_statement_params(statement, context)) {
		return *err;
	}

	auto sys_like_id = find_by_statement<ecsact_system_like_id>( //
		package_id,
		*context
	);

	if(ecsact::meta::system_notify_settings_count(*sys_like_id) > 0) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_MULTIPLE_NOTIFY_STATEMENTS,
			.relevant_content = {},
		};
	}

	auto data = statement.data.system_notify_statement;

	auto setting_name = std::string_view( //
		data.setting_name.data,
		data.setting_name.length
	);

	if(!setting_name.empty()) {
		auto notify_setting = get_notify_setting_from_string(setting_name);

		if(!notify_setting) {
			return ecsact_eval_error{
				.code = ECSACT_EVAL_ERR_INVALID_NOTIFY_SETTING,
				.relevant_content = data.setting_name,
			};
		}

		for(auto&& [comp_id, _] : ecsact::meta::system_capabilities(*sys_like_id)) {
			ecsact_set_system_notify_component_setting(
				*sys_like_id,
				comp_id,
				*notify_setting
			);
		}
	}

	return {};
}

static auto eval_system_notify_component_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) -> ecsact_eval_error {
	if(context_stack.size() < 2) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	auto [context, err] =
		expect_context(context_stack, {ECSACT_STATEMENT_SYSTEM_NOTIFY});

	if(err.code != ECSACT_EVAL_OK) {
		return err;
	}

	if(auto err = disallow_statement_params(statement, context)) {
		return *err;
	}

	auto block_setting_name = std::string_view( //
		context->data.system_notify_statement.setting_name.data,
		context->data.system_notify_statement.setting_name.length
	);

	auto data = statement.data.system_notify_component_statement;

	if(!block_setting_name.empty()) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_NOTIFY_BLOCK_AND_COMPONENTS,
			.relevant_content = data.setting_name,
			.context_type = context->type,
		};
	}

	auto sys_like_id = find_by_statement<ecsact_system_like_id>( //
		package_id,
		context_stack[context_stack.size() - 2]
	);

	auto comp_like_name = std::string( //
		data.component_name.data,
		data.component_name.length
	);

	auto comp_like_id = find_by_name<ecsact_component_like_id>( //
		package_id,
		comp_like_name
	);

	if(!comp_like_id) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_UNKNOWN_COMPONENT_LIKE_TYPE,
			.relevant_content = data.component_name,
		};
	}

	auto setting_name = std::string_view( //
		data.setting_name.data,
		data.setting_name.length
	);

	auto notify_setting = get_notify_setting_from_string(setting_name);

	if(!notify_setting) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_NOTIFY_SETTING,
			.relevant_content = data.setting_name,
		};
	}

	for(auto&& [existing_comp_id, _] :
			ecsact::meta::system_notify_settings(*sys_like_id)) {
		if(existing_comp_id == *comp_like_id) {
			return ecsact_eval_error{
				.code = ECSACT_EVAL_ERR_DUPLICATE_NOTIFY_COMPONENT,
				.relevant_content = {},
			};
		}
	}

	ecsact_set_system_notify_component_setting(
		*sys_like_id,
		*comp_like_id,
		*notify_setting
	);

	return {};
}

static ecsact_eval_error eval_entity_constraint_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) {
	if(context_stack.size() < 2) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	auto& generates_statement = context_stack[context_stack.size() - 1];
	auto& sys_like_statement = context_stack[context_stack.size() - 2];

	if(generates_statement.type != ECSACT_STATEMENT_SYSTEM_GENERATES) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	if(auto err = disallow_statement_params(statement, &generates_statement)) {
		return *err;
	}

	auto sys_like_id =
		find_by_statement<ecsact_system_like_id>(package_id, sys_like_statement);

	if(!sys_like_id) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	auto& data = statement.data.entity_constraint_statement;

	std::string comp_name(
		data.constraint_component_name.data,
		data.constraint_component_name.length
	);

	auto comp_id = find_by_name<ecsact_component_id>(package_id, comp_name);
	if(!comp_id) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_UNKNOWN_COMPONENT_TYPE,
			.relevant_content = data.constraint_component_name,
		};
	}

	auto sys_gen_ids = ecsact::meta::get_system_generates_ids(*sys_like_id);
	if(sys_gen_ids.empty()) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	auto gen_comps =
		ecsact::meta::get_system_generates_components(*sys_like_id, sys_gen_ids[0]);

	for(auto& entry : gen_comps) {
		if(entry.first == *comp_id) {
			return ecsact_eval_error{
				.code = ECSACT_EVAL_ERR_GENERATES_DUPLICATE_COMPONENT_CONSTRAINTS,
				.relevant_content = data.constraint_component_name,
			};
		}
	}

	ecsact_system_generates_set_component(
		*sys_like_id,
		sys_gen_ids[0],
		*comp_id,
		data.optional ? ECSACT_SYS_GEN_OPTIONAL : ECSACT_SYS_GEN_REQUIRED
	);

	return {};
}

ecsact_eval_error ecsact_eval_statement(
	ecsact_package_id       package_id,
	int32_t                 statement_stack_size,
	const ecsact_statement* statement_stack
) {
	if(statement_stack_size == 0) {
		return {};
	}

	auto&     statement = statement_stack[statement_stack_size - 1];
	std::span context_statements(statement_stack, statement_stack_size - 1);

	auto err = [&]() -> std::optional<ecsact_eval_error> {
		switch(statement.type) {
			case ECSACT_STATEMENT_NONE:
				return eval_none_statement(package_id, context_statements, statement);
			case ECSACT_STATEMENT_UNKNOWN:
				return eval_unknown_statement(
					package_id,
					context_statements,
					statement
				);
			case ECSACT_STATEMENT_PACKAGE:
				return ecsact_eval_error{
					.code = ECSACT_EVAL_ERR_UNEXPECTED_STATEMENT,
					.relevant_content = {},
				};
			case ECSACT_STATEMENT_IMPORT:
				return eval_import_statement(package_id, context_statements, statement);
			case ECSACT_STATEMENT_COMPONENT:
				return eval_component_statement(
					package_id,
					context_statements,
					statement
				);
			case ECSACT_STATEMENT_TRANSIENT:
				return eval_transient_statement(
					package_id,
					context_statements,
					statement
				);
			case ECSACT_STATEMENT_SYSTEM:
				return eval_system_statement(package_id, context_statements, statement);
			case ECSACT_STATEMENT_ACTION:
				return eval_action_statement(package_id, context_statements, statement);
			case ECSACT_STATEMENT_ENUM:
				return eval_enum_statement(package_id, context_statements, statement);
			case ECSACT_STATEMENT_ENUM_VALUE:
				return eval_enum_value_statement(
					package_id,
					context_statements,
					statement
				);
			case ECSACT_STATEMENT_BUILTIN_TYPE_FIELD:
				return eval_builtin_type_field_statement(
					package_id,
					context_statements,
					statement
				);
			case ECSACT_STATEMENT_USER_TYPE_FIELD:
				return eval_user_type_field_statement(
					package_id,
					context_statements,
					statement
				);
			case ECSACT_STATEMENT_ENTITY_FIELD:
				return eval_entity_field_statement(
					package_id,
					context_statements,
					statement
				);
			case ECSACT_STATEMENT_SYSTEM_COMPONENT:
				return eval_system_component_statement(
					package_id,
					context_statements,
					statement
				);
			case ECSACT_STATEMENT_SYSTEM_GENERATES:
				return eval_system_generates_statement(
					package_id,
					context_statements,
					statement
				);
			case ECSACT_STATEMENT_SYSTEM_WITH:
				return eval_system_with_statement(
					package_id,
					context_statements,
					statement
				);
			case ECSACT_STATEMENT_ENTITY_CONSTRAINT:
				return eval_entity_constraint_statement(
					package_id,
					context_statements,
					statement
				);
			case ECSACT_STATEMENT_SYSTEM_NOTIFY:
				return eval_system_notify_statement(
					package_id,
					context_statements,
					statement
				);
			case ECSACT_STATEMENT_SYSTEM_NOTIFY_COMPONENT:
				return eval_system_notify_component_statement(
					package_id,
					context_statements,
					statement
				);
		}

		return std::nullopt;
	}();

	if(!err) {
		assert(false && "Unhandled Statement Type");
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INTERNAL,
			.relevant_content = {},
			.context_type = context_statements.empty()
				? ECSACT_STATEMENT_NONE
				: context_statements.back().type,
		};
	}

	return *err;
}

ecsact_package_id ecsact_eval_package_statement(
	const ecsact_package_statement* package_statement
) {
	return ecsact_create_package(
		package_statement->main,
		package_statement->package_name.data,
		package_statement->package_name.length
	);
}

void ecsact_eval_reset() {
}

void ecsact::detail::check_file_eval_error(
	ecsact_eval_error&      in_out_error,
	ecsact_package_id       package_id,
	ecsact_parse_status     status,
	const ecsact_statement& statement,
	const std::string&      source
) {
	assert(in_out_error.code == ECSACT_EVAL_OK);

	if(status.code == ECSACT_PARSE_STATUS_BLOCK_END) {
		if(statement.type == ECSACT_STATEMENT_ACTION) {
			auto& data = statement.data.action_statement;

			auto act_id = find_by_name<ecsact_action_id>(
				package_id,
				std::string(data.action_name.data, data.action_name.length)
			);

			auto caps = ecsact::meta::system_capabilities(*act_id);
			if(caps.empty()) {
				in_out_error.code = ECSACT_EVAL_ERR_NO_CAPABILITIES;
				in_out_error.relevant_content = {
					.data = source.c_str(),
					.length = static_cast<int32_t>(source.size()),
				};
			}
		}
	}
}
