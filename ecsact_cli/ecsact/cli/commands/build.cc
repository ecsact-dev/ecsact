#include "ecsact/cli/commands/build.hh"

#include <iostream>
#include <memory>
#include <format>
#include <filesystem>
#include <array>
#include <variant>
#include <string_view>
#include <format>
#include "docopt.h"
#include "magic_enum.hpp"
#include "ecsact/interpret/eval.hh"
#include "ecsact/runtime/dynamic.h"
#include "ecsact/runtime/meta.hh"
#include "ecsact/cli/report.hh"
#include "ecsact/cli/detail/argv0.hh"
#include "ecsact/cli/commands/common.hh"
#include "ecsact/cli/commands/build/recipe/taste.hh"
#include "ecsact/cli/commands/build/build_recipe.hh"
#include "ecsact/cli/commands/build/cc_compiler.hh"
#include "ecsact/cli/commands/build/recipe/cook.hh"
#include "ecsact/cli/commands/build/recipe/taste.hh"
#include "ecsact/cli/commands/recipe-bundle/build_recipe_bundle.hh"

namespace fs = std::filesystem;

using namespace std::string_view_literals;

constexpr auto USAGE = R"docopt(Ecsact Build Command

Usage:
  ecsact build (-h | --help)
  ecsact build <files>... --recipe=<name>... --output=<path> [--allow-unresolved-imports] [--format=<type>] [--temp_dir=<path>] [--compiler_config=<path>] [--report_filter=<filter>] [--debug] [--tracy]

Options:
  <files>                   Ecsact files used to build Ecsact Runtime
  -r --recipe=<name>        Name or path to recipe
  -o --output=<path>        Runtime output path
  --temp_dir=<path>         Optional temporary directory to use instead of generated one
  --compiler_config=<path>  Optionally specify the compiler by name or path
  -f --format=<type>        The format used to report progress of the build [default: text]
  --report_filter=<filter>  Filtering out report logs [default: none]
  --debug                   Compile with debug symbols
	--tracy                   Enable the tracy profiler
)docopt";

// TODO(zaucy): Add this documentation to docopt (msvc regex fails)
// Allowed Formats:
//   none : No output
//   json : Each line of stdout/stderr is a JSON object
//   text : Human readable text format

constexpr auto allowed_recipe_extensions = std::array{
	""sv,
	".ecsact-recipe-bundle"sv,
	".yml"sv,
	".yaml"sv,
	".json"sv, // json works since yaml is a superset
};

auto resolve_builtin_recipe( //
	std::string recipe_str,
	const char* argv[]
) -> std::optional<fs::path> {
	using namespace std::string_literals;

	auto exec_path = ecsact::cli::detail::canon_argv0(argv[0]);
	auto install_prefix = exec_path.parent_path().parent_path();
	auto recipes_dir = install_prefix / "share" / "ecsact" / "recipes";

	if(fs::exists(recipes_dir)) {
		for(auto& entry : fs::directory_iterator(recipes_dir)) {
			auto filename = entry.path().filename().replace_extension("").string();
			const auto prefix = "ecsact_"s;

			auto stripped_filename = filename.substr(prefix.size(), filename.size());

			if(recipe_str == stripped_filename) {
				auto path = fs::path(filename);
				path.replace_extension(".ecsact-recipe-bundle");
				return recipes_dir / path;
			}
		}
	}
	return std::nullopt;
}

auto ecsact::cli::detail::build_command( //
	int         argc,
	const char* argv[]
) -> int {
	auto args = docopt::docopt(USAGE, {argv + 1, argv + argc});
	auto exec_path = canon_argv0(argv[0]);

	if(auto exit_code = process_common_args(args); exit_code != 0) {
		return exit_code;
	}

	auto temp_dir = args["--temp_dir"].isString() //
		? fs::path{args["--temp_dir"].asString()}
		: fs::temp_directory_path();

	auto files = args.at("<files>").asStringList();
	auto file_paths = std::vector<fs::path>{};
	file_paths.reserve(files.size());

	auto output_path = fs::path{args.at("--output").asString()};

	auto recipe_composite = std::optional<build_recipe>{};
	auto recipe_paths = args.at("--recipe").asStringList();
	for(auto& recipe_path_str : recipe_paths) {
		auto builtin_path = resolve_builtin_recipe(recipe_path_str, argv);

		fs::path recipe_path;
		if(builtin_path) {
			recipe_path = *builtin_path;
		} else {
			recipe_path = fs::path{recipe_path_str};
		}

		if(std::ranges::find(allowed_recipe_extensions, recipe_path.extension()) ==
			 allowed_recipe_extensions.end()) {
			ecsact::cli::report_error(
				"Invalid recipe file extension {}",
				recipe_path.extension().string()
			);
			return 1;
		}

		if(!recipe_path.has_extension()) {
			recipe_path.replace_extension("ecsact-recipe-bundle");
		}

		if(recipe_path.extension() == ".ecsact-recipe-bundle") {
			auto bundle_result = ecsact::build_recipe_bundle::from_file(recipe_path);

			if(!bundle_result) {
				ecsact::cli::report_error(
					"Failed to open build recipe bundle {}: {}",
					recipe_path.generic_string(),
					bundle_result.error().what()
				);
				return 1;
			}

			auto extract_result = bundle_result->extract(temp_dir);
			if(!extract_result) {
				ecsact::cli::report_error(
					"Failed to extract build recipe bundle {}: {}",
					recipe_path.generic_string(),
					extract_result.error().what()
				);
				return 1;
			}

			recipe_path = extract_result.recipe_path();
			recipe_path_str = recipe_path.generic_string();

			ecsact::cli::report_info(
				"Extracted build recipe bundle to {}",
				recipe_path.parent_path().generic_string()
			);
		}

		auto recipe_result = build_recipe::from_yaml_file(recipe_path);

		if(std::holds_alternative<build_recipe_parse_error>(recipe_result)) {
			auto recipe_error = std::get<build_recipe_parse_error>(recipe_result);
			ecsact::cli::report_error(
				"Recipe Error {}",
				magic_enum::enum_name(recipe_error)
			);
			return 1;
		}

		auto recipe = std::move(std::get<build_recipe>(recipe_result));
		if(!recipe_composite) {
			recipe_composite.emplace(std::move(recipe));
		} else {
			auto merge_result = build_recipe::merge(*recipe_composite, recipe);
			if(std::holds_alternative<build_recipe_merge_error>(merge_result)) {
				auto merge_error = std::get<build_recipe_merge_error>(merge_result);
				ecsact::cli::report_error(
					"Recipe Merge Error {}",
					magic_enum::enum_name(merge_error)
				);
				return 1;
			}

			recipe_composite.emplace(std::get<build_recipe>(std::move(merge_result)));
		}
	}

	if(!recipe_composite) {
		ecsact::cli::report_error("No recipe");
		return 1;
	}

	if(!args["--allow-unresolved-imports"].asBool()) {
		if(!recipe_composite->imports().empty()) {
			for(auto imp : recipe_composite->imports()) {
				ecsact::cli::report_error("Unresolved import {}", imp);
			}
			ecsact::cli::report_error(
				"Build recipes do not resolve all imports. Make sure all imported "
				"functions in provided recipes are also exported by another recipe. "
				"If you would like to allow unresolved imports you may provide the "
				"--allow-unresolved-imports flag to suppress this error."
			);
			return 1;
		}
	}

	// TODO(zaucy): Generate stable directory based on file input
	auto work_dir = temp_dir / "ecsact-build";

	ecsact::cli::report_info(
		"Build Working Directory: {}",
		work_dir.generic_string()
	);

	auto ec = std::error_code{};
	fs::remove_all(work_dir, ec);
	if(ec) {
		ecsact::cli::report_error(
			"Failed to clear work directory: {}",
			ec.message()
		);
		return 1;
	}
	fs::create_directories(work_dir, ec);

	for(auto file : files) {
		if(!fs::exists(file)) {
			ecsact::cli::report_error("Input file {} does not exist\n", file);
			return 1;
		}

		file_paths.emplace_back(file);
	}

	if(auto main_pkg_id = ecsact::meta::main_package(); main_pkg_id) {
		ecsact_destroy_package(*main_pkg_id);
	}
	auto eval_errors = ecsact::eval_files(file_paths);

	if(!eval_errors.empty()) {
		for(auto eval_error : eval_errors) {
			auto err_source_path = file_paths[eval_error.source_index];
			ecsact::cli::report(ecsact_error_message{
				.ecsact_source_path = err_source_path.generic_string(),
				.message = eval_error.error_message,
				.line = eval_error.line,
				.character = eval_error.character,
			});
		}

		return 1;
	}

	auto main_pkg_id = ecsact_meta_main_package();

	if(main_pkg_id == static_cast<ecsact_package_id>(-1)) {
		ecsact::cli::report_error("ecsact build must have main package");
		return 1;
	}

	ecsact::cli::report_info(
		"Main ecsact package: {}",
		ecsact::meta::package_name(main_pkg_id)
	);

	auto compiler_config_path = args["--compiler_config"].isString() //
		? std::optional{fs::path{args["--compiler_config"].asString()}}
		: std::nullopt;

	auto compiler = compiler_config_path //
		? ecsact::cli::load_compiler_config(*compiler_config_path)
		: ecsact::cli::detect_cc_compiler(work_dir);

	if(!compiler) {
		ecsact::cli::report_error(
			"Failed to detect C++ compiler installed on your system"
		);
		return 1;
	}

	ecsact::cli::report_info(
		"Using compiler: {} ({})",
		to_string(compiler->compiler_type),
		compiler->compiler_version
	);

	auto additional_plugin_dirs = std::vector<fs::path>{};
	for(fs::path recipe_path : recipe_paths) {
		if(recipe_path.has_parent_path()) {
			additional_plugin_dirs.emplace_back(recipe_path.parent_path());
		}
	}

	auto cook_options = cook_recipe_options{
		.files = file_paths,
		.work_dir = work_dir,
		.output_path = output_path,
		.debug = args["--debug"].asBool(),
		.tracy = args["--tracy"].asBool(),
		.additional_plugin_dirs = additional_plugin_dirs,
	};
	auto runtime_output_path =
		cook_recipe(argv[0], *recipe_composite, *compiler, cook_options);

	if(!runtime_output_path) {
		ecsact::cli::report_error("Failed to cook recipe");
		return 1;
	}

	auto exit_code = ecsact::cli::taste_recipe( //
		*recipe_composite,
		*runtime_output_path
	);

	if(exit_code == 0) {
		ecsact::cli::report_success(
			"Successfully cooked and tasted {}",
			output_path.generic_string()
		);
	}

	return exit_code;
}
