#include "ecsact/cli/commands/build/build_recipe.hh"

#include <fstream>
#include <filesystem>
#include <cassert>
#include <sstream>
#include <string>
#include <string_view>
#include <ranges>
#include <iostream>
#include <algorithm>
#include "yaml-cpp/yaml.h"
#include "ecsact/cli/commands/build/get_modules.hh"
#include "ecsact/cli/commands/codegen/codegen_util.hh"
#include "ecsact/cli/report.hh"

namespace fs = std::filesystem;
using namespace std::string_literals;
using namespace std::string_view_literals;

namespace YAML {
static auto operator<<( //
	YAML::Emitter&                            out,
	const ecsact::build_recipe::source_fetch& src
) -> YAML::Emitter& {
	out << YAML::BeginMap;
	out << YAML::Key << "fetch";
	out << YAML::Value << src.url;
	if(src.outdir) {
		out << YAML::Key << "outdir";
		out << YAML::Value << *src.outdir;
	}
	out << YAML::EndMap;
	return out;
}

static auto operator<<( //
	YAML::Emitter&                           out,
	const ecsact::build_recipe::source_path& src
) -> YAML::Emitter& {
	if(src.outdir) {
		out << YAML::BeginMap;
		out << YAML::Key << "path";
		out << YAML::Value << src.path.generic_string();
		out << YAML::Key << "outdir";
		out << YAML::Value << *src.outdir;
		out << YAML::EndMap;
	} else {
		out << src.path.generic_string();
	}
	return out;
}

static auto operator<<( //
	YAML::Emitter&                              out,
	const ecsact::build_recipe::source_codegen& src
) -> YAML::Emitter& {
	out << YAML::BeginMap;
	out << YAML::Key << "codegen";
	if(src.plugins.size() > 1) {
		out << YAML::Value << src.plugins;
	} else if(!src.plugins.empty()) {
		out << YAML::Value << src.plugins[0];
	}
	if(src.outdir) {
		out << YAML::Key << "outdir";
		out << YAML::Value << *src.outdir;
	}
	out << YAML::EndMap;
	return out;
}

static auto operator<<( //
	YAML::Emitter&                      out,
	const ecsact::build_recipe::source& src
) -> YAML::Emitter& {
	return std::visit(
		[&](const auto& src) -> YAML::Emitter& { return out << src; },
		src
	);
}
} // namespace YAML

static auto range_contains(auto& r, const auto& v) -> bool {
	for(auto& rv : r) {
		if(rv == v) {
			return true;
		}
	}
	return false;
}

ecsact::build_recipe::build_recipe() = default;
ecsact::build_recipe::build_recipe(build_recipe&&) = default;
ecsact::build_recipe::build_recipe(const build_recipe&) = default;
ecsact::build_recipe::~build_recipe() = default;

auto ecsact::build_recipe::name() const -> const std::string_view {
	return _name.empty() ? "(unnamed)"sv : _name;
}

auto ecsact::build_recipe::base_directory() const -> std::filesystem::path {
	return _base_directory;
}

auto ecsact::build_recipe::exports() const -> std::span<const std::string> {
	return _exports;
}

auto ecsact::build_recipe::imports() const -> std::span<const std::string> {
	return _imports;
}

auto ecsact::build_recipe::sources() const -> std::span<const source> {
	return _sources;
}

auto ecsact::build_recipe::system_libs() const -> std::span<const std::string> {
	return _system_libs;
}

auto ecsact::build_recipe::defines() const
	-> std::unordered_map<std::string, std::string> {
	return _defines;
}

static auto get_if_error( //
	const auto& result
) -> std::optional<ecsact::build_recipe_parse_error> {
	if(auto err = std::get_if<ecsact::build_recipe_parse_error>(&result)) {
		return *err;
	} else {
		return {};
	}
}

static auto get_value(auto& result) {
	auto value = std::get_if<0>(&result);
	// If you're using get_value you should have already did get_if_error
	assert(value != nullptr);
	return *value;
}

static auto parse_exports( //
	YAML::Node exports
) -> std::variant<std::vector<std::string>, ecsact::build_recipe_parse_error> {
	if(!exports.IsSequence()) {
		return ecsact::build_recipe_parse_error::missing_exports;
	}

	return exports.as<std::vector<std::string>>();
}

static auto parse_imports( //
	YAML::Node imports
) -> std::variant<std::vector<std::string>, ecsact::build_recipe_parse_error> {
	if(!imports) {
		return {};
	}

	return imports.as<std::vector<std::string>>();
}

static auto parse_system_libs( //
	YAML::Node system_libs
) -> std::variant<std::vector<std::string>, ecsact::build_recipe_parse_error> {
	if(!system_libs) {
		return {};
	}

	return system_libs.as<std::vector<std::string>>();
}

static auto parse_defines( //
	YAML::Node defines
)
	-> std::variant<
		std::unordered_map<std::string, std::string>,
		ecsact::build_recipe_parse_error> {
	if(!defines) {
		return {};
	}

	return defines.as<std::unordered_map<std::string, std::string>>();
}

static auto parse_sources( //
	fs::path   recipe_path,
	YAML::Node sources
)
	-> std::variant<
		std::vector<ecsact::build_recipe::source>,
		ecsact::build_recipe_parse_error> {
	using source = ecsact::build_recipe::source;
	using source_fetch = ecsact::build_recipe::source_fetch;
	using source_codegen = ecsact::build_recipe::source_codegen;
	using source_path = ecsact::build_recipe::source_path;

	auto result = std::vector<ecsact::build_recipe::source>{};

	for(auto src : sources) {
		if(src.IsMap()) {
			auto codegen = src["codegen"];
			auto fetch = src["fetch"];
			auto path = src["path"];

			if((codegen && fetch) || (codegen && path) || (fetch && path) ||
				 (!codegen && !fetch && !path)) {
				return ecsact::build_recipe_parse_error::invalid_source;
			}

			if(codegen) {
				auto entry = source_codegen{};
				if(src["outdir"]) {
					entry.outdir = src["outdir"].as<std::string>();
				}
				if(codegen.IsSequence()) {
					entry.plugins = codegen.as<std::vector<std::string>>();
				} else {
					entry.plugins.push_back(codegen.as<std::string>());
				}
				result.emplace_back(entry);
			} else if(fetch) {
				auto outdir = std::optional<std::string>{};
				auto integrity = std::optional<std::string>{};
				auto strip_prefix = std::optional<std::string>{};
				auto paths = std::optional<std::vector<std::string>>{};
				if(src["integrity"]) {
					integrity = src["integrity"].as<std::string>();
					if(integrity->empty()) {
						integrity = {};
					}
				}
				if(src["strip_prefix"]) {
					strip_prefix = src["strip_prefix"].as<std::string>();
					if(strip_prefix->empty()) {
						strip_prefix = {};
					}
				}
				if(src["paths"]) {
					paths = src["paths"].as<std::vector<std::string>>();
				}
				if(src["outdir"]) {
					outdir = src["outdir"].as<std::string>();
				}
				result.emplace_back(
					source_fetch{
						.url = fetch.as<std::string>(),
						.integrity = integrity,
						.strip_prefix = strip_prefix,
						.outdir = outdir,
						.paths = paths,
					}
				);
			} else if(path) {
				auto src_path = fs::path{path.as<std::string>()};
				auto outdir = std::optional<std::string>{};
				auto relative_to_cwd = src["relative_to_cwd"] //
					? src["relative_to_cwd"].as<bool>()
					: false;
				if(src["outdir"]) {
					outdir = src["outdir"].as<std::string>();
				}

				if(relative_to_cwd && !src_path.is_absolute()) {
					src_path = (fs::current_path() / src_path).lexically_normal();
				} else {
					src_path = src_path.lexically_normal();
				}

				result.emplace_back(
					source_path{
						.path = src_path,
						.outdir = outdir,
					}
				);
			}
		} else if(src.IsScalar()) {
			auto path = fs::path{src.as<std::string>()}.lexically_normal();
			result.emplace_back(
				source_path{
					.path = path,
					.outdir = ".",
				}
			);
		}
	}

	return result;
}

auto ecsact::build_recipe::build_recipe_from_yaml_node( //
	YAML::Node doc,
	fs::path   p
) -> create_result {
	if(!doc.IsMap()) {
		return ecsact::build_recipe_parse_error::expected_map_top_level;
	}

	auto exports = parse_exports(doc["exports"]);
	if(auto err = get_if_error(exports)) {
		return *err;
	}

	auto imports = parse_imports(doc["imports"]);
	if(auto err = get_if_error(imports)) {
		return *err;
	}

	auto sources = parse_sources(p, doc["sources"]);
	if(auto err = get_if_error(sources)) {
		return *err;
	}

	auto system_libs = parse_system_libs(doc["system_libs"]);
	if(auto err = get_if_error(system_libs)) {
		return *err;
	}

	auto defines = parse_defines(doc["defines"]);
	if(auto err = get_if_error(defines)) {
		return *err;
	}

	auto recipe = ecsact::build_recipe{};
	if(doc["name"]) {
		recipe._name = doc["name"].as<std::string>();
	}
	if(p.has_parent_path()) {
		recipe._base_directory = p.parent_path().generic_string();
	}
	recipe._exports = get_value(exports);
	recipe._imports = get_value(imports);
	recipe._sources = get_value(sources);
	recipe._system_libs = get_value(system_libs);
	recipe._defines = get_value(defines);

	if(recipe._exports.empty()) {
		return build_recipe_parse_error::missing_exports;
	}

	auto import_modules =
		ecsact::cli::detail::get_ecsact_modules(recipe._imports);
	auto export_modules =
		ecsact::cli::detail::get_ecsact_modules(recipe._exports);

	if(!import_modules.unknown_module_methods.empty()) {
		return build_recipe_parse_error::unknown_import_method;
	}

	if(!export_modules.unknown_module_methods.empty()) {
		return build_recipe_parse_error::unknown_export_method;
	}

	for(auto&& [imp_mod, _] : import_modules.module_methods) {
		for(auto&& [exp_mod, _] : export_modules.module_methods) {
			if(imp_mod == exp_mod) {
				return build_recipe_parse_error::
					conflicting_import_export_method_modules;
			}
		}
	}

	return recipe;
}

auto ecsact::build_recipe::from_yaml_string( //
	const std::string& str,
	fs::path           p
) -> create_result {
	auto doc = YAML::Load(str);
	return build_recipe_from_yaml_node(doc, p);
}

auto ecsact::build_recipe::from_yaml_file( //
	std::filesystem::path p
) -> create_result {
	auto file = std::ifstream{p};
	try {
		auto doc = YAML::LoadFile(p.string());
		return build_recipe_from_yaml_node(doc, p);
	} catch(const YAML::BadFile& err) {
		ecsact::cli::report_error("YAML PARSE: {}", err.what());
		return build_recipe_parse_error::bad_file;
	}
}

auto ecsact::build_recipe::to_yaml_string() const -> std::string {
	auto out = std::stringstream{};
	auto emitter = YAML::Emitter{out};

	emitter << YAML::BeginMap;

	emitter << YAML::Key << "imports";
	emitter << YAML::Value << _imports;

	emitter << YAML::Key << "exports";
	emitter << YAML::Value << _exports;

	emitter << YAML::Key << "system_libs";
	emitter << YAML::Value << _system_libs;

	emitter << YAML::Key << "defines";
	emitter << YAML::Value << YAML::BeginMap;
	for(auto&& [key, value] : _defines) {
		emitter << YAML::Key << key << YAML::Value << value;
	}
	emitter << YAML::EndMap;

	emitter << YAML::Key << "sources";
	emitter << YAML::Value << _sources;

	emitter << YAML::EndMap;

	return out.str();
}

auto ecsact::build_recipe::to_yaml_bytes() const -> std::vector<std::byte> {
	auto str = to_yaml_string();
	static_assert(sizeof(std::byte) == sizeof(char));
	return std::vector<std::byte>{
		reinterpret_cast<std::byte*>(str.data()),
		reinterpret_cast<std::byte*>(str.data()) + str.size(),
	};
}

auto ecsact::build_recipe::merge( //
	const build_recipe& base,
	const build_recipe& target
) -> merge_result {
	auto merged_build_recipe = build_recipe{};

	for(auto base_export_name : base._exports) {
		for(auto target_export_name : target._exports) {
			if(target_export_name == base_export_name) {
				ecsact::cli::report_error(
					"Multiple recipes export {}",
					target_export_name
				);
				return build_recipe_merge_error::conflicting_export;
			}
		}
	}

	merged_build_recipe._base_directory = base._base_directory;
	merged_build_recipe._name =
		std::format("{} + {}", base.name(), target.name());

	merged_build_recipe._system_libs.reserve(
		merged_build_recipe._system_libs.size() + target._system_libs.size()
	);
	merged_build_recipe._system_libs.insert(
		merged_build_recipe._system_libs.end(),
		base._system_libs.begin(),
		base._system_libs.end()
	);
	merged_build_recipe._system_libs.insert(
		merged_build_recipe._system_libs.end(),
		target._system_libs.begin(),
		target._system_libs.end()
	);

	merged_build_recipe._defines.reserve(
		merged_build_recipe._defines.size() + target._defines.size()
	);

	for(auto&& [k, v] : base._defines) {
		merged_build_recipe._defines[k] = v;
	}

	for(auto&& [k, v] : target._defines) {
		merged_build_recipe._defines[k] = v;
	}

	merged_build_recipe._exports.reserve(
		merged_build_recipe._exports.size() + target._exports.size()
	);
	merged_build_recipe._exports.insert(
		merged_build_recipe._exports.end(),
		base._exports.begin(),
		base._exports.end()
	);
	merged_build_recipe._exports.insert(
		merged_build_recipe._exports.end(),
		target._exports.begin(),
		target._exports.end()
	);

	merged_build_recipe._imports.reserve(
		merged_build_recipe._imports.size() + target._imports.size()
	);
	merged_build_recipe._imports.insert(
		merged_build_recipe._imports.end(),
		base._imports.begin(),
		base._imports.end()
	);
	merged_build_recipe._imports.insert(
		merged_build_recipe._imports.end(),
		target._imports.begin(),
		target._imports.end()
	);

	for(auto itr = merged_build_recipe._imports.begin();
			itr != merged_build_recipe._imports.end();) {
		if(range_contains(merged_build_recipe._exports, *itr)) {
			itr = merged_build_recipe._imports.erase(itr);
			continue;
		}
		++itr;
	}

	merged_build_recipe._sources.reserve(
		merged_build_recipe._sources.size() + target._sources.size()
	);
	merged_build_recipe._sources.insert(
		merged_build_recipe._sources.end(),
		base._sources.begin(),
		base._sources.end()
	);

	for(auto& src : target._sources) {
		if(std::holds_alternative<source_path>(src)) {
			source_path src_path = std::get<source_path>(src);
			if(!src_path.path.is_absolute()) {
				src_path.path = fs::proximate(
					target.base_directory() / src_path.path,
					base.base_directory()
				);
			}

			merged_build_recipe._sources.emplace_back(std::move(src_path));
		} else if(std::holds_alternative<source_codegen>(src)) {
			source_codegen src_codegen = std::get<source_codegen>(src);
			for(auto& plugin : src_codegen.plugins) {
				if(!cli::is_default_plugin(plugin) && !fs::path{plugin}.is_absolute()) {
					plugin =
						fs::relative(target.base_directory() / plugin).generic_string();
				}
			}

			merged_build_recipe._sources.emplace_back(src_codegen);
		} else {
			merged_build_recipe._sources.push_back(src);
		}
	}

	return merged_build_recipe;
}

auto ecsact::build_recipe::copy() const -> build_recipe {
	return *this;
}

auto ecsact::build_recipe::update_sources( //
	std::span<const source> new_sources
) -> void {
	_sources.clear();
	_sources.insert(_sources.begin(), new_sources.begin(), new_sources.end());
}
