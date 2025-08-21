#pragma once

#include <string>
#include <variant>
#include <vector>
#include <span>
#include <filesystem>
#include <cstddef>
#include <optional>
#include <filesystem>
#include <unordered_map>

namespace YAML {
class Node;
}

namespace ecsact {

enum class build_recipe_parse_error {
	bad_file,
	missing_exports,
	expected_map_top_level,
	invalid_source,
	unknown_import_method,
	unknown_export_method,
	conflicting_import_export_method_modules,
};

enum class build_recipe_merge_error {
	conflicting_export,
};

class build_recipe {
public:
	struct create_result;
	struct merge_result;

	static auto from_yaml_file( //
		std::filesystem::path p
	) -> create_result;

	static auto from_yaml_string( //
		const std::string&    str,
		std::filesystem::path p
	) -> create_result;

	static auto merge( //
		const build_recipe& a,
		const build_recipe& b
	) -> merge_result;

	struct source_path {
		std::filesystem::path      path;
		std::optional<std::string> outdir;
	};

	struct source_fetch {
		std::string                             url;
		std::optional<std::string>              integrity;
		std::optional<std::string>              strip_prefix;
		std::optional<std::string>              outdir;
		std::optional<std::vector<std::string>> paths;
	};

	struct source_codegen {
		std::vector<std::string>   plugins;
		std::optional<std::string> outdir;
	};

	using source = std::variant<source_path, source_fetch, source_codegen>;

	build_recipe(build_recipe&&);
	~build_recipe();

	auto name() const -> const std::string_view;
	auto base_directory() const -> std::filesystem::path;
	auto exports() const -> std::span<const std::string>;
	auto imports() const -> std::span<const std::string>;
	auto sources() const -> std::span<const source>;
	auto system_libs() const -> std::span<const std::string>;
	auto defines() const -> std::unordered_map<std::string, std::string>;

	auto to_yaml_string() const -> std::string;
	auto to_yaml_bytes() const -> std::vector<std::byte>;

	auto update_sources(std::span<const source> new_sources) -> void;
	auto copy() const -> build_recipe;

private:
	std::string              _name;
	std::string              _base_directory;
	std::vector<std::string> _exports;
	std::vector<std::string> _imports;
	std::vector<source>      _sources;
	std::vector<std::string> _system_libs;

	std::unordered_map<std::string, std::string> _defines;

	build_recipe();
	build_recipe(const build_recipe&);

	static auto build_recipe_from_yaml_node( //
		YAML::Node            doc,
		std::filesystem::path p
	) -> create_result;
};

struct build_recipe::create_result
	: std::variant<build_recipe, build_recipe_parse_error> {
	using variant::variant;
};

struct build_recipe::merge_result
	: std::variant<build_recipe, build_recipe_merge_error> {
	using variant::variant;
};
} // namespace ecsact
