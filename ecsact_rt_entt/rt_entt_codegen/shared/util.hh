#pragma once

#include <string_view>
#include <vector>
#include <string>
#include <utility>
#include <ranges>
#include <format>
#include "ecsact/codegen/plugin.hh"
#include "ecsact/lang-support/lang-cc.hh"
#include "ecsact/runtime/meta.h"
#include "ecsact/runtime/meta.hh"
#include "ecsact_entt_details.hh"

namespace ecsact::rt_entt_codegen::util {

class global_initializer_printer {
	bool                            disposed = false;
	ecsact::codegen_plugin_context& ctx;

public:
	global_initializer_printer(
		ecsact::codegen_plugin_context& ctx,
		auto&&                          global_name
	)
		: ctx(ctx) {
		ctx.write(
			"decltype(ecsact::entt::detail::globals::",
			global_name,
			") ",
			"ecsact::entt::detail::globals::",
			global_name,
			" = ",
			"[]() -> "
			"std::remove_cvref_t<decltype(ecsact::entt::detail::globals::",
			global_name,
			")> {"
		);
		ctx.indentation += 1;
		ctx.write("\n");
		ctx.write(
			"auto result = "
			"std::remove_cvref_t<decltype(ecsact::entt::detail::globals::",
			global_name,
			")>{};\n"
		);
	}

	global_initializer_printer(global_initializer_printer&& other)
		: ctx(other.ctx) {
		disposed = other.disposed;
		other.disposed = true;
	}

	~global_initializer_printer() {
		if(disposed) {
			return;
		}
		disposed = true;
		ctx.write("return result;");
		ctx.indentation -= 1;
		ctx.write("\n}();\n\n");
	}
};

template<typename DeclId>
inline auto decl_cpp_ident(DeclId id) -> std::string {
	using ecsact::cc_lang_support::cpp_identifier;
	using ecsact::meta::decl_full_name;
	return "::" + cpp_identifier(decl_full_name(id));
}

template<typename CompositeID>
inline auto has_no_fields(CompositeID id) {
	return ecsact_meta_count_fields(ecsact_id_cast<ecsact_composite_id>(id)) == 0;
}

inline auto init_global( //
	ecsact::codegen_plugin_context& ctx,
	auto&&                          global_name
) -> void {
	// globals can only be initialized at the top
	assert(ctx.indentation == 0);

	ctx.write(
		"decltype(ecsact::entt::detail::globals::",
		global_name,
		") ecsact::entt::detail::globals::",
		global_name,
		" = {};\n"
	);
}

inline auto init_global( //
	ecsact::codegen_plugin_context& ctx,
	auto&&                          global_name,
	auto&&                          body_fn
) -> void {
	auto printer = global_initializer_printer{ctx, global_name};
	body_fn();
}

inline auto inc_header( //
	ecsact::codegen_plugin_context& ctx,
	auto&&                          header_path
) -> void {
	ctx.write("#include \"", header_path, "\"\n");
}

inline auto inc_package_header( //
	ecsact::codegen_plugin_context& ctx,
	ecsact_package_id               pkg_id
) -> void {
	namespace fs = std::filesystem;

	auto main_ecsact_file_path =
		ecsact::meta::package_file_path(ctx.package_id).lexically_normal();

	if(ctx.package_id == pkg_id) {
		main_ecsact_file_path.replace_extension(
			main_ecsact_file_path.extension().string() + ".hh"
		);

		inc_header(ctx, main_ecsact_file_path.filename().string());
	} else {
		auto cpp_header_path = ecsact::meta::package_file_path(pkg_id);
		cpp_header_path.replace_extension(
			cpp_header_path.extension().string() + ".hh"
		);
		if(main_ecsact_file_path.has_parent_path()) {
			cpp_header_path =
				fs::relative(cpp_header_path, main_ecsact_file_path.parent_path());
		}
		inc_header(ctx, cpp_header_path.filename().string());
	}
}

inline auto is_transient_component(
	ecsact_package_id        package_id,
	ecsact_component_like_id comp_id
) -> bool {
	auto transient_ids = ecsact::meta::get_transient_ids(package_id);
	for(auto transient_id : transient_ids) {
		auto transient_component_like_id =
			ecsact_id_cast<ecsact_component_like_id>(transient_id);
		if(transient_component_like_id == comp_id) {
			return true;
		}
	}
	return false;
}

class method_printer {
	using parameters_list_t = std::vector<std::pair<std::string, std::string>>;

	bool                             disposed = false;
	std::optional<parameters_list_t> parameters;
	ecsact::codegen_plugin_context&  ctx;

	auto _parameter(std::string param_type, std::string param_name) -> void {
		assert(!disposed);
		if(disposed) {
			return;
		}

		assert(
			parameters.has_value() &&
			"Cannot set parameters after return type has been set"
		);
		parameters->push_back({param_type, param_name});
	}

	auto _return_type(std::string type) {
		assert(!disposed);
		if(disposed) {
			return;
		}

		assert(parameters.has_value());
		if(!parameters.has_value()) {
			return;
		}

		if(!parameters->empty()) {
			ctx.write("\n");
		}

		for(auto i = 0; parameters->size() > i; ++i) {
			auto&& [param_type, param_name] = parameters->at(i);
			ctx.write("\t", param_type, " ", param_name);
			if(i + 1 < parameters->size()) {
				ctx.write(",");
			}
			ctx.write("\n");
		}

		parameters = std::nullopt;

		ctx.write(") -> ", type, " {");
		ctx.indentation += 1;
		ctx.write("\n");
	}

public:
	method_printer( //
		ecsact::codegen_plugin_context& ctx,
		std::string                     method_name
	)
		: ctx(ctx) {
		parameters.emplace();
		ctx.write("auto ", method_name, "(");
	}

	method_printer(method_printer&& other) : ctx(other.ctx) {
		disposed = other.disposed;

		if(!disposed) {
			parameters = std::move(other.parameters);
		}

		other.disposed = true;
	}

	auto parameter(
		std::string param_type,
		std::string param_name
	) & -> method_printer& {
		_parameter(param_type, param_name);
		return *this;
	}

	auto parameter(
		std::string param_type,
		std::string param_name
	) && -> method_printer {
		_parameter(param_type, param_name);
		return std::move(*this);
	}

	auto return_type(std::string type) & -> method_printer& {
		_return_type(type);
		return *this;
	}

	auto return_type(std::string type) && -> method_printer {
		_return_type(type);
		return std::move(*this);
	}

	~method_printer() {
		if(disposed) {
			return;
		}
		disposed = true;
		ctx.indentation -= 1;
		ctx.write("\n}\n\n");
	}
};

auto comma_delim(auto&& range) -> std::string {
	auto result = std::string{};

	for(auto str : range) {
		result += str + ", ";
	}

	if(result.ends_with(", ")) {
		result = result.substr(0, result.size() - 2);
	}

	return result;
}

auto make_view( //
	ecsact::codegen_plugin_context&                            ctx,
	auto&&                                                     view_var_name,
	auto&&                                                     registry_var_name,
	const ecsact::rt_entt_codegen::ecsact_entt_system_details& details,
	std::vector<std::string> additional_components = {},
	std::vector<std::string> additional_exclude_components = {}
) -> void {
	using namespace std::string_literals;
	using ecsact::rt_entt_codegen::util::comma_delim;
	using ecsact::rt_entt_codegen::util::decl_cpp_ident;
	using std::views::transform;

	ctx.write("auto ", view_var_name, " = ", registry_var_name, ".view<");

	ctx.write(comma_delim(
		details.get_comps | transform(decl_cpp_ident<ecsact_component_like_id>)
	));

	for(auto comp_id : details.writable_comps) {
		auto comp_name = decl_cpp_ident(comp_id);

		additional_components.push_back(std::format(
			"ecsact::entt::detail::exec_beforechange_storage<{}>",
			comp_name
		));
	}

	if(!additional_components.empty()) {
		ctx.write(", ");
		ctx.write(comma_delim(additional_components));
	}

	ctx.write(">(");

	auto exclude_comps = details.exclude_comps |
		transform(decl_cpp_ident<ecsact_component_like_id>);

	additional_exclude_components.insert(
		additional_exclude_components.end(),
		exclude_comps.begin(),
		exclude_comps.end()
	);

	if(!additional_exclude_components.empty()) {
		ctx.write(
			"::entt::exclude<",
			comma_delim(additional_exclude_components),
			">"
		);
	}

	ctx.write(");\n");
}

template<typename ID>
constexpr auto ecsact_id_type_to_string() -> std::string_view;

template<>
constexpr auto ecsact_id_type_to_string<ecsact_component_like_id>()
	-> std::string_view {
	return "ecsact_component_like_id";
}

/**
 * Get the smallest and largest ID by number.
 * Used when creating and id_map
 */
template<typename ID>
auto ecsact_id_min_max() -> std::tuple<ID, ID>;

template<>
auto ecsact_id_min_max<ecsact_component_id>()
	-> std::tuple<ecsact_component_id, ecsact_component_id>;

template<>
auto ecsact_id_min_max<ecsact_transient_id>()
	-> std::tuple<ecsact_transient_id, ecsact_transient_id>;

template<>
auto ecsact_id_min_max<ecsact_component_like_id>()
	-> std::tuple<ecsact_component_like_id, ecsact_component_like_id>;

/**
 * Helper function for printing an ecsact::entt::detail::id_map in codegen
 */
template<typename ID>
auto make_id_map_type(std::string value_type) -> std::string {
	auto id_type_str = ecsact_id_type_to_string<ID>();
	auto [min, max] = ecsact_id_min_max<ID>();
	return std::format(
		"::ecsact::entt::detail::id_map<{0}, {1}, "
		"static_cast<{0}>({2}), "
		"static_cast<{0}>({3})>",
		id_type_str,
		value_type,
		static_cast<int>(min),
		static_cast<int>(max)
	);
}
} // namespace ecsact::rt_entt_codegen::util
