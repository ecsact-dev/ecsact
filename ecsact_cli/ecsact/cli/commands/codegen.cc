#include "./codegen.hh"

#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <string_view>
#include <unordered_set>
#include <boost/dll/shared_library.hpp>
#include <boost/dll/library_info.hpp>
#include "docopt.h"
#include "ecsact/interpret/eval.hh"
#include "ecsact/cli/commands/codegen/codegen.hh"
#include "ecsact/cli/commands/common.hh"
#include "ecsact/cli/report.hh"
#include "ecsact/runtime/dynamic.h"
#include "ecsact/runtime/meta.hh"
#include "ecsact/runtime/dylib.h"
#include "ecsact/codegen/plugin.h"
#include "ecsact/codegen/plugin_validate.hh"
#include "ecsact/cli/commands/codegen/codegen_util.hh"

namespace fs = std::filesystem;
constexpr auto file_readonly_perms = fs::perms::others_read |
	fs::perms::group_read | fs::perms::owner_read;

constexpr auto USAGE = R"(Ecsact Codegen Command

Usage:
  ecsact codegen <files>... --plugin=<plugin> [--stdout]
  ecsact codegen <files>... --plugin=<plugin>... [--outdir=<directory>] [--format=<type>] [--report_filter=<filter>] [--print-output-files]

Options:
  -p, --plugin=<plugin>     Name of bundled plugin or path to plugin.
  --stdout                  Print to stdout instead of writing to a file. May only be used if a single ecsact file and single ecsact codegen plugin are used.
  -o, --outdir=<directory>  Specify directory generated files should be written to. By default generated files are written next to source files.
  -f --format=<type>        The format used to report progress of the build [default: text]
  --report_filter=<filter>  Filtering out report logs [default: none]
  --print-output-files      Simply print output file paths to stdout. No codegen will occur.
)";

static auto stdout_write_fn(
	int32_t     filename_index,
	const char* str,
	int32_t     str_len
) -> void {
	std::cout << std::string_view(str, str_len);
}

int ecsact::cli::detail::codegen_command(int argc, const char* argv[]) {
	using namespace std::string_literals;

	auto args = docopt::docopt(USAGE, {argv + 1, argv + argc});
	if(auto exit_code = process_common_args(args); exit_code != 0) {
		return exit_code;
	}

	auto only_print_output_files = args.at("--print-output-files").asBool();

	auto files_error = false;
	auto files_str = args.at("<files>").asStringList();
	auto files = std::vector<fs::path>{};
	files.reserve(files_str.size());

	for(fs::path file_path : files_str) {
		files.push_back(file_path);
		if(file_path.extension() != ".ecsact") {
			files_error = true;
			report_error(
				"Expected .ecsact file instead got '{}'",
				file_path.string()
			);
		} else if(!fs::exists(file_path)) {
			files_error = true;
			report_error("'{}' does not exist", file_path.string());
		}
	}

	auto default_plugins_dir = get_default_plugins_dir();
	auto plugin_paths = std::vector<fs::path>{};
	auto plugins = std::vector<boost::dll::shared_library>{};
	auto plugins_not_found = false;
	auto invalid_plugins = false;

	for(auto plugin_arg : args.at("--plugin").asStringList()) {
		auto checked_plugin_paths = std::vector<fs::path>{};
		auto plugin_path = resolve_plugin_path(
			{
				.plugin_arg = plugin_arg,
				.default_plugins_dir = default_plugins_dir,
				.additional_plugin_dirs = {},
			},
			checked_plugin_paths
		);

		if(plugin_path) {
			std::error_code ec;
			auto&           plugin = plugins.emplace_back();
			plugin.load(plugin_path->string(), ec);
			auto validate_result = ecsact::codegen::plugin_validate(*plugin_path);
			if(validate_result.ok()) {
				plugin_paths.emplace_back(*plugin_path);
			} else {
				invalid_plugins = true;
				plugins.pop_back();
				auto err_msg = "Plugin validation failed for '" + plugin_arg + "'\n";
				for(auto err : validate_result.errors) {
					err_msg += " - "s + to_string(err) + "\n";
				}
				report_error("{}", err_msg);
			}
		} else {
			plugins_not_found = true;
			auto err_msg =
				"Unable to find codegen plugin '" + plugin_arg + "'. Paths checked:\n";
			for(auto& checked_path : checked_plugin_paths) {
				err_msg += " - " + fs::relative(checked_path).string() + "\n";
			}
			report_error("{}", err_msg);
		}
	}

	if(invalid_plugins || plugins_not_found || files_error) {
		return 1;
	}

	if(auto main_pkg_id = ecsact::meta::main_package(); main_pkg_id) {
		ecsact_destroy_package(*main_pkg_id);
	}
	auto eval_errors = ecsact::eval_files(files);
	if(!eval_errors.empty()) {
		for(auto& eval_err : eval_errors) {
			report_error(
				"{}:{}:{} {}",
				files[eval_err.source_index].string(),
				eval_err.line,
				eval_err.character,
				eval_err.error_message
			);
		}
		return 1;
	}

	std::optional<fs::path> outdir;
	if(args.at("--outdir").isString()) {
		outdir = fs::path(args.at("--outdir").asString());
		if(!fs::exists(*outdir)) {
			std::error_code ec;
			fs::create_directories(*outdir, ec);
			if(ec) {
				report_error(
					"Failed to create out directory {}: {}",
					outdir->string(),
					ec.message()
				);
				return 3;
			}
		}
	}

	auto codegen_options = ecsact::cli::codegen_options{
		.plugin_paths = plugin_paths,
		.outdir = outdir,
		.only_print_output_files = only_print_output_files,
	};

	if(args.at("--stdout").asBool()) {
		codegen_options.write_fn = &stdout_write_fn;
	}

	auto exit_code = ecsact::cli::codegen(codegen_options);

	if(args.at("--stdout").asBool()) {
		std::cout.flush();
	}

	return exit_code;
}
