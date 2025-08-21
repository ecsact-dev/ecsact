#include <vector>
#include <string>
#include <cassert>
#include <filesystem>
#include <string_view>
#include <ranges>
#include <set>
#include <format>
#include "ecsact/runtime/meta.hh"
#include "ecsact/codegen/plugin.h"
#include "ecsact/codegen/plugin.hh"
#include "ecsact/lang-support/lang-cc.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"

namespace fs = std::filesystem;
using namespace ecsact::cpp_codegen_plugin_util;

using ecsact::cc_lang_support::anonymous_system_name;
using ecsact::cc_lang_support::c_identifier;
using ecsact::cc_lang_support::cpp_identifier;
using ecsact::meta::get_action_ids;
using ecsact::meta::get_system_generates_ids;
using ecsact::meta::get_system_ids;

constexpr auto GENERATED_FILE_DISCLAIMER = R"(// GENERATED FILE - DO NOT EDIT
)";

static void word_wrap(std::string& str, int max_length) {
	int index = 0;
	int count = 0;
	int last_word_end = 0;

	for(auto itr = str.begin(); itr != str.end(); ++itr) {
		if(count >= max_length) {
			str[last_word_end] = '\n';
			count = 0;
		} else if(*itr == '\n') {
			count = 0;
		} else {
			count += 1;
		}

		if(std::isspace(*itr)) {
			last_word_end = index;
		}

		index += 1;
	}
}

static void insert_prefix(std::string& str, std::string_view prefix) {
	for(auto itr = str.begin(); itr != str.end(); ++itr) {
		if(*itr == '\n' && itr != str.begin()) {
			itr = str.insert(std::next(itr), prefix.begin(), prefix.end());
		}
	}
}

static auto write_static_assert_codegen_error(
	ecsact::codegen_plugin_context& ctx,
	std::string_view                err_title,
	std::string                     err_msg,
	std::string                     constexpr_err_type = "T"
) -> void {
	word_wrap(err_msg, 78);

	ctx.writef("// local type to make static assert always fail\n");
	ctx.writef("struct codegen_error {{}};\n");
	ctx.writef(
		"static_assert(std::is_same_v<{}, codegen_error>, ",
		constexpr_err_type
	);
	ctx.writef("\n\"| [Ecsact C++ Error]: {}\\n\"", err_title);
	for(auto line : err_msg | std::views::split('\n')) {
		ctx.writef("\n\"| {}\\n\"", std::string_view{line.begin(), line.end()});
	}
	ctx.writef(");");
}

static auto write_context_method_error_body(
	ecsact::codegen_plugin_context&           ctx,
	std::string_view                          err_msg,
	const std::set<ecsact_component_like_id>& allowed_components
) -> void {
	auto full_err_msg = std::string(err_msg);
	full_err_msg += " The following components are allowed:\n";

	for(auto comp_like_id : allowed_components) {
		full_err_msg += "\t- " + ecsact::meta::decl_full_name(comp_like_id) + "\n";
	}

	write_static_assert_codegen_error(
		ctx,
		"System Execution Context Misuse",
		full_err_msg
	);
}

static auto write_context_get_decl(
	ecsact::codegen_plugin_context&           ctx,
	std::string_view                          sys_like_full_name,
	const std::set<ecsact_component_like_id>& gettable_components
) -> void {
	ctx.writef("template<typename T, typename... AssocFields>\n");
	block(ctx, "auto get(AssocFields&&... assoc_fields) -> T", [&] {
		write_context_method_error_body(
			ctx,
			std::format(
				"{} context.get<T> may only be called with a component readable by the "
				"system. Did you forget to add readonly or readwrite capabilities?",
				sys_like_full_name
			),
			gettable_components
		);
	});
	ctx.writef("\n\n");
}

static auto write_context_update_decl(
	ecsact::codegen_plugin_context&           ctx,
	std::string_view                          sys_like_full_name,
	const std::set<ecsact_component_like_id>& updatable_components
) -> void {
	ctx.writef("template<typename T, typename... AssocFields>\n");
	block(
		ctx,
		"auto update(const T& updated_component, AssocFields&&... assoc_fields) -> "
		"void",
		[&] {
			write_context_method_error_body(
				ctx,
				std::format(
					"{} context.update<T> may only be called with a component writable "
					"by "
					"the system. Did you forget to add readwrite capabilities?",
					sys_like_full_name
				),
				updatable_components
			);
		}
	);
	ctx.writef("\n\n");
}

static auto write_context_add_decl(
	ecsact::codegen_plugin_context&           ctx,
	std::string_view                          sys_like_full_name,
	const std::set<ecsact_component_like_id>& addable_components
) -> void {
	ctx.writef("template<typename T>\n");
	ctx.writef("\trequires(!std::is_empty_v<T>)\n");
	block(ctx, "auto add(const T& new_component) -> void", [&] {
		write_context_method_error_body(
			ctx,
			std::format(
				"{} context.add<T> may only be called with a component addable by the "
				"system. Did you forget to add adds capabilities?",
				sys_like_full_name
			),
			addable_components
		);
	});
	ctx.writef("\n");

	ctx.writef("template<typename T>\n");
	block(ctx, "auto add() -> void", [&] {
		write_context_method_error_body(
			ctx,
			std::format(
				"{} context.add<T> may only be called with a component addable by the "
				"system. Did you forget to add adds capabilities?",
				sys_like_full_name
			),
			addable_components
		);
	});
	ctx.writef("\n\n");
}

static auto write_context_remove_decl(
	ecsact::codegen_plugin_context&           ctx,
	std::string_view                          sys_like_full_name,
	const std::set<ecsact_component_like_id>& removable_components
) -> void {
	ctx.writef("template<typename T, typename... AssocFields>\n");
	block(ctx, "auto remove(AssocFields&&... assoc_fields) -> void", [&] {
		write_context_method_error_body(
			ctx,
			std::format(
				"{} context.remove<T> may only be called with a component removable by "
				"the system. Did you forget to add removes capabilities?",
				sys_like_full_name
			),
			removable_components
		);
	});
	ctx.writef("\n\n");
}

static auto write_context_has_decl(
	ecsact::codegen_plugin_context&           ctx,
	std::string_view                          sys_like_full_name,
	const std::set<ecsact_component_like_id>& optional_components
) -> void {
	ctx.writef("template<typename T, typename... AssocFields>\n");
	block(ctx, "auto has(AssocFields&&... assoc_fields) -> bool", [&] {
		write_context_method_error_body(
			ctx,
			std::format(
				"{} context.has<T> may only be called with optional components "
				"declared by the system. Did you forget to add opitonal capabilities?",
				sys_like_full_name
			),
			optional_components
		);
	});
	ctx.writef("\n\n");
}

static auto write_context_stream_toggle_decl(
	ecsact::codegen_plugin_context&           ctx,
	std::string_view                          sys_like_full_name,
	const std::set<ecsact_component_like_id>& stream_components
) -> void {
	ctx.write("template<typename T, typename... AssocFields>\n");
	block(
		ctx,
		"auto stream_toggle(bool stream_enable, AssocFields&&... assoc_fields) -> "
		"void",
		[&] {
			write_context_method_error_body(
				ctx,
				std::format(
					"{} context.stream_toggle<T> may only be called with stream "
					"components "
					"declared by the system. Did you forget to add stream toggle "
					"capabilities?",
					sys_like_full_name
				),
				stream_components
			);
		}
	);
	ctx.write("\n\n");
}

static auto write_context_other_decl(
	ecsact::codegen_plugin_context&     ctx,
	std::vector<ecsact_system_assoc_id> assoc_ids
) -> void {
	if(assoc_ids.empty()) {
		return;
	}

	auto avail_indices_str = comma_delim(
		std::views::iota(0UL, assoc_ids.size()) |
		std::views::transform([](auto i) { return std::to_string(i); })
	);

	ctx.writef(
		"template<std::size_t Index{}>\n",
		assoc_ids.size() == 1 ? " = 0" : ""
	);
	block(ctx, "auto other() -> other_context<Index>", [&] {
		write_static_assert_codegen_error(
			ctx,
			"System Execution Context Misuse",
			std::format( //
				"context.other<>() must be called with an index. Available "
				"indices for this system are {}",
				avail_indices_str
			),
			"other_context<Index>"
		);
	});

	ctx.writef("\n");
}

static void write_context_action(
	ecsact::codegen_plugin_context& ctx,
	ecsact_action_id                act_id
) {
	auto full_name = ecsact::meta::decl_full_name(act_id);
	auto cpp_full_name = cpp_identifier(full_name);

	block(ctx, std::format("auto action() const -> {}", cpp_full_name), [&] {
		ctx.writef("return _ctx.action<{}>();", cpp_full_name);
	});
	ctx.writef("\n");
}

static void write_context_entity(ecsact::codegen_plugin_context& ctx) {
	block(ctx, "auto entity() const -> ecsact_entity_id", [&] {
		ctx.writef("return _ctx.entity();");
	});
	ctx.writef("\n");
}

template<typename CompositeID>
static auto assoc_field_ids( //
	CompositeID compo_id
) -> std::vector<ecsact_field_id> {
	auto result = std::vector<ecsact_field_id>{};
	for(auto field_id : ecsact::meta::get_field_ids(compo_id)) {
		auto field_type = ecsact::meta::get_field_type(compo_id, field_id);

		if(field_type.kind == ECSACT_TYPE_KIND_BUILTIN) {
			if(field_type.type.builtin == ECSACT_ENTITY_TYPE) {
				result.push_back(field_id);
				continue;
			}
		}

		if(field_type.kind == ECSACT_TYPE_KIND_FIELD_INDEX) {
			result.push_back(field_id);
			continue;
		}
	}
	return result;
}

template<typename CompositeID>
static auto assoc_field_names_only_str(CompositeID compo_id) -> std::string {
	auto result = std::string{};
	for(auto field_id : assoc_field_ids(compo_id)) {
		auto field_type = ecsact::meta::get_field_type(compo_id, field_id);
		auto field_name = ecsact::meta::field_name(compo_id, field_id);
		auto cpp_field_type_name =
			ecsact::cc_lang_support::cpp_field_type_name(field_type);
		result +=
			std::format("std::forward<{0}>({1}), ", cpp_field_type_name, field_name);
	}
	if(!result.empty()) {
		result = result.substr(0, result.size() - 2); // strip off ", "
	}
	return result;
}

template<typename CompositeID>
static auto assoc_field_types_only_str(CompositeID compo_id) -> std::string {
	auto result = std::string{};
	for(auto field_id : assoc_field_ids(compo_id)) {
		auto field_type = ecsact::meta::get_field_type(compo_id, field_id);
		auto cpp_field_type_name =
			ecsact::cc_lang_support::cpp_field_type_name(field_type);
		result += std::format("{}, ", cpp_field_type_name);
	}
	if(!result.empty()) {
		result = result.substr(0, result.size() - 2); // strip off ", "
	}
	return result;
}

template<typename CompositeID>
static auto assoc_fields_str(CompositeID compo_id) -> std::string {
	auto result = std::string{};
	for(auto field_id : assoc_field_ids(compo_id)) {
		auto field_type = ecsact::meta::get_field_type(compo_id, field_id);
		auto field_name = ecsact::meta::field_name(compo_id, field_id);
		auto cpp_field_type_name =
			ecsact::cc_lang_support::cpp_field_type_name(field_type);
		result += std::format("{}&& {}, ", cpp_field_type_name, field_name);
	}
	if(!result.empty()) {
		result = result.substr(0, result.size() - 2); // strip off ", "
	}
	return result;
}

static void write_context_get_specialize(
	ecsact::codegen_plugin_context& ctx,
	ecsact_component_like_id        comp_id
) {
	using ecsact::cc_lang_support::cpp_identifier;

	auto decl_id = ecsact_id_cast<ecsact_decl_id>(comp_id);
	auto full_name = ecsact_meta_decl_full_name(decl_id);
	auto cpp_full_name = cpp_identifier(full_name);
	auto assoc_fields = assoc_fields_str(comp_id);
	auto assoc_field_types_only = assoc_field_types_only_str(comp_id);
	auto assoc_field_names_only = assoc_field_names_only_str(comp_id);

	if(!assoc_field_types_only.empty()) {
		assoc_field_types_only = ", " + assoc_field_types_only;
	}

	block(
		ctx,
		std::format(
			"template<> auto get<{0}{1}>({2}) -> {0}",
			cpp_full_name,
			assoc_field_types_only,
			assoc_fields
		),
		[&] {
			ctx.writef(
				"return _ctx.get<{0}{1}>({2});",
				cpp_full_name,
				assoc_field_types_only,
				assoc_field_names_only
			);
		}
	);

	ctx.writef("\n");
}

static void write_context_add_specialize(
	ecsact::codegen_plugin_context& ctx,
	ecsact_component_like_id        comp_id
) {
	auto decl_id = ecsact_id_cast<ecsact_decl_id>(comp_id);
	auto full_name = ecsact_meta_decl_full_name(decl_id);
	auto cpp_full_name = cpp_identifier(full_name);
	auto field_count =
		ecsact_meta_count_fields(ecsact_id_cast<ecsact_composite_id>(comp_id));

	if(field_count > 0) {
		block(
			ctx,
			std::format(
				"template<> auto add<{0}>(const {0}& new_component) -> void",
				cpp_full_name
			),
			[&] { ctx.writef("_ctx.add<{}>(new_component);", cpp_full_name); }
		);
	} else {
		block(
			ctx,
			std::format("template<> auto add<{}>() -> void", cpp_full_name),
			[&] { ctx.writef("_ctx.add<{}>();", cpp_full_name); }
		);
	}

	ctx.writef("\n");
}

static auto write_context_update_specialize(
	ecsact::codegen_plugin_context& ctx,
	ecsact_component_like_id        comp_id
) -> void {
	auto decl_id = ecsact_id_cast<ecsact_decl_id>(comp_id);
	auto full_name = ecsact_meta_decl_full_name(decl_id);
	auto cpp_full_name = cpp_identifier(full_name);
	auto assoc_fields = assoc_fields_str(comp_id);
	auto assoc_field_types_only = assoc_field_types_only_str(comp_id);
	auto assoc_field_names_only = assoc_field_names_only_str(comp_id);

	if(!assoc_field_types_only.empty()) {
		assoc_field_types_only = ", " + assoc_field_types_only;
	}

	if(!assoc_fields.empty()) {
		assoc_fields = ", " + assoc_fields;
	}

	if(!assoc_field_names_only.empty()) {
		assoc_field_names_only = ", " + assoc_field_names_only;
	}

	block(
		ctx,
		std::format(
			"template<> auto update<{0}{1}>(const {0}& updated_component{2}) -> "
			"void",
			cpp_full_name,
			assoc_field_types_only,
			assoc_fields
		),
		[&] {
			ctx.writef(
				"_ctx.update<{0}>(updated_component{1});",
				cpp_full_name,
				assoc_field_names_only
			);
		}
	);
	ctx.writef("\n");
}

static auto write_context_remove_specialize(
	ecsact::codegen_plugin_context& ctx,
	ecsact_component_like_id        comp_id
) -> void {
	auto decl_id = ecsact_id_cast<ecsact_decl_id>(comp_id);
	auto full_name = ecsact_meta_decl_full_name(decl_id);
	auto cpp_full_name = cpp_identifier(full_name);

	block(
		ctx,
		std::format("template<> auto remove<{}>() -> void", cpp_full_name),
		[&] { ctx.writef("return _ctx.remove<{}>();\n", cpp_full_name); }
	);

	ctx.writef("\n");
}

static auto write_context_stream_toggle_specialize(
	ecsact::codegen_plugin_context& ctx,
	ecsact_component_like_id        comp_id
) -> void {
	auto decl_id = ecsact_id_cast<ecsact_decl_id>(comp_id);
	auto full_name = ecsact_meta_decl_full_name(decl_id);
	auto cpp_full_name = cpp_identifier(full_name);

	block(
		ctx,
		std::format(
			"template<> auto stream_toggle<{}>(bool enable_stream) -> void",
			cpp_full_name
		),
		[&] {
			ctx.write(std::format(
				"return _ctx.stream_toggle<{}>(enable_stream);\n",
				cpp_full_name
			));
		}
	);

	ctx.write("\n");
}

static auto write_context_other_specialize(
	ecsact::codegen_plugin_context& ctx,
	ecsact_system_like_id           system_like_id,
	ecsact_system_assoc_id          assoc_id,
	size_t                          assoc_index
) -> void {
	auto full_name = ecsact::meta::decl_full_name(system_like_id);
	auto c_impl_fn_name = c_identifier(full_name);
	auto c_impl_assoc_id_name =
		std::format("{}__{}", c_impl_fn_name, assoc_index);
	block(
		ctx,
		std::format(
			"template<> auto other<{0}>() -> other_context<{0}>",
			assoc_index
		),
		[&] {
			block(ctx, std::format("return other_context<{}>", assoc_index), [&] {
				ctx.writef( //
					"._ctx = _ctx.other({}),",
					c_impl_assoc_id_name
				);
			});
			ctx.writef(";");
		}
	);
}

struct context_body_details {
	std::set<ecsact_component_like_id> add_components;
	std::set<ecsact_component_like_id> get_components;
	std::set<ecsact_component_like_id> update_components;
	std::set<ecsact_component_like_id> remove_components;
	std::set<ecsact_component_like_id> optional_components;
	std::set<ecsact_component_like_id> stream_components;

	static auto from_caps(auto caps) -> context_body_details {
		auto details = context_body_details{};

		for(auto&& [comp_id, cap] : caps) {
			if((cap & ECSACT_SYS_CAP_READONLY) == ECSACT_SYS_CAP_READONLY) {
				details.get_components.emplace(comp_id);
			}

			if((cap & ECSACT_SYS_CAP_WRITEONLY) == ECSACT_SYS_CAP_WRITEONLY) {
				details.update_components.emplace(comp_id);
			}

			if((cap & ECSACT_SYS_CAP_ADDS) == ECSACT_SYS_CAP_ADDS) {
				details.add_components.emplace(comp_id);
			}

			if((cap & ECSACT_SYS_CAP_REMOVES) == ECSACT_SYS_CAP_REMOVES) {
				details.remove_components.emplace(comp_id);
			}

			if((cap & ECSACT_SYS_CAP_OPTIONAL) == ECSACT_SYS_CAP_OPTIONAL) {
				details.optional_components.emplace(comp_id);
			}

			if((cap & ECSACT_SYS_CAP_STREAM_TOGGLE) == ECSACT_SYS_CAP_STREAM_TOGGLE) {
				details.stream_components.emplace(comp_id);
			}
		}

		return details;
	}

	static auto from_assoc_id( //
		ecsact_system_like_id  sys_like_id,
		ecsact_system_assoc_id assoc_id
	) -> context_body_details {
		auto caps = ecsact::meta::system_assoc_capabilities(sys_like_id, assoc_id);
		auto details = from_caps(caps);
		return details;
	}

	static auto from_sys_like( //
		ecsact_system_like_id sys_like_id
	) -> context_body_details {
		auto caps = ecsact::meta::system_capabilities(sys_like_id);
		auto details = from_caps(caps);
		return details;
	}
};

/**
 * Write execution context body methods that are common between systems,
 * actions, and other (entity association) contexts.
 */
static auto write_context_body_common(
	ecsact::codegen_plugin_context& ctx,
	std::string                     ctx_name,
	context_body_details            details
) -> void {
	ctx.writef("[[no_unique_address]] ::ecsact::execution_context _ctx;\n\n");

	if(!details.get_components.empty()) {
		write_context_get_decl(ctx, ctx_name, details.get_components);
	}
	if(!details.update_components.empty()) {
		write_context_update_decl(ctx, ctx_name, details.update_components);
	}
	if(!details.add_components.empty()) {
		write_context_add_decl(ctx, ctx_name, details.add_components);
	}
	if(!details.remove_components.empty()) {
		write_context_remove_decl(ctx, ctx_name, details.remove_components);
	}
	if(!details.optional_components.empty()) {
		write_context_has_decl(ctx, ctx_name, details.optional_components);
	}

	if(!details.stream_components.empty()) {
		write_context_stream_toggle_decl(ctx, ctx_name, details.stream_components);
	}

	for(auto get_comp_id : details.get_components) {
		write_context_get_specialize(ctx, get_comp_id);
	}

	for(auto add_comp_id : details.add_components) {
		write_context_add_specialize(ctx, add_comp_id);
	}

	for(auto update_comp_id : details.update_components) {
		write_context_update_specialize(ctx, update_comp_id);
	}

	for(auto remove_comp_id : details.remove_components) {
		write_context_remove_specialize(ctx, remove_comp_id);
	}

	for(auto stream_comp_id : details.stream_components) {
		write_context_stream_toggle_specialize(ctx, stream_comp_id);
	}

	write_context_entity(ctx);
}

template<typename ID>
static auto write_sys_context(
	ecsact::codegen_plugin_context& ctx,
	ID                              id,
	auto&&                          extra_body_fn
) {
	constexpr bool is_action = std::is_same_v<ecsact_action_id, ID>;
	auto           sys_like_id = ecsact_id_cast<ecsact_system_like_id>(id);
	std::string    full_name =
		ecsact_meta_decl_full_name(ecsact_id_cast<ecsact_decl_id>(id));

	if constexpr(!is_action) {
		if(full_name.empty()) {
			full_name += ecsact::meta::package_name(ctx.package_id) + ".";
			full_name += anonymous_system_name(id);
		}
	} else {
		assert(!full_name.empty());
	}

	auto assoc_ids = ecsact::meta::system_assoc_ids(sys_like_id);

	ctx.writef("\n");
	block(ctx, std::format("struct {}::context", cpp_identifier(full_name)), [&] {
		if(!assoc_ids.empty()) {
			ctx.writef("template<std::size_t Index>\n");
			ctx.writef("struct other_context;\n");
		}

		for(auto i = 0; assoc_ids.size() > i; ++i) {
			ctx.writef("\n");
			block(ctx, std::format("template<> struct other_context<{}>", i), [&] {
				write_context_body_common(
					ctx,
					std::format("{} (assoc {})", full_name, i),
					context_body_details::from_assoc_id(sys_like_id, assoc_ids[i])
				);
			});
			ctx.writef(";\n\n");
		}

		std::optional<ecsact_system_like_id> parent_sys_like_id;
		if constexpr(!is_action) {
			parent_sys_like_id = ecsact::meta::get_parent_system_id(id);
		}

		write_context_body_common(
			ctx,
			full_name,
			context_body_details::from_sys_like(sys_like_id)
		);

		auto gen_ids = get_system_generates_ids(sys_like_id);

		write_context_other_decl(ctx, assoc_ids);

		if(parent_sys_like_id) {
			auto parent_full_name = ecsact::meta::decl_full_name(*parent_sys_like_id);
			if(parent_full_name.empty()) {
				parent_full_name += ecsact::meta::package_name(ctx.package_id) + ".";
				parent_full_name += anonymous_system_name(*parent_sys_like_id);
			}
			auto parent_cpp_full_name = cpp_identifier(parent_full_name);
			ctx.writef("const {}::context parent() const;\n", parent_cpp_full_name);
		}

		ctx.writef("\n\n");

		for(auto i = 0; assoc_ids.size() > i; ++i) {
			write_context_other_specialize(ctx, sys_like_id, assoc_ids[i], i);
		}

		extra_body_fn();
	});
	ctx.writef(";\n");
};

void ecsact_codegen_plugin(
	ecsact_package_id          package_id,
	ecsact_codegen_write_fn_t  write_fn,
	ecsact_codegen_report_fn_t report_fn
) {
	ecsact::codegen_plugin_context ctx{package_id, 0, write_fn, report_fn};

	ctx.writef(GENERATED_FILE_DISCLAIMER);
	ctx.writef("#pragma once\n\n");

	fs::path package_hh_path = ecsact_meta_package_file_path(package_id);
	fs::path package_systems_h_path = package_hh_path;
	package_hh_path.replace_extension(
		package_hh_path.extension().string() + ".hh"
	);
	package_systems_h_path.replace_extension(
		package_systems_h_path.extension().string() + ".systems.h"
	);

	ctx.writef("#include <type_traits>\n");
	ctx.writef("#include \"ecsact/cpp/execution_context.hh\"\n");
	ctx.writef("#include \"{}\"\n", package_hh_path.filename().string());
	ctx.writef("#include \"{}\"\n", package_systems_h_path.filename().string());

	for(auto dep_pkg_id : ecsact::meta::get_dependencies(package_id)) {
		fs::path dep_pkg_systems_hh_path =
			ecsact_meta_package_file_path(dep_pkg_id);
		dep_pkg_systems_hh_path.replace_extension(
			dep_pkg_systems_hh_path.extension().string() + ".systems.hh"
		);

		if(dep_pkg_systems_hh_path.has_parent_path()) {
			dep_pkg_systems_hh_path =
				fs::relative(dep_pkg_systems_hh_path, package_hh_path.parent_path());
		} else {
			dep_pkg_systems_hh_path = dep_pkg_systems_hh_path.filename();
		}

		ctx.writef("#include \"{}\"\n", dep_pkg_systems_hh_path.generic_string());
	}

	ctx.writef("\n");

	ctx.writef("\nstruct ecsact_system_execution_context;\n");

	for(auto sys_id : get_system_ids(ctx.package_id)) {
		write_sys_context(ctx, sys_id, [] {});
	}

	for(auto act_id : get_action_ids(ctx.package_id)) {
		write_sys_context(ctx, act_id, [&] { write_context_action(ctx, act_id); });
	}
}
