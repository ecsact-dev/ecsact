#include "ecsact/cli/commands/build/recipe/taste.hh"

#include <format>
#include <algorithm>
#include <memory>
#include <filesystem>
#include <iostream>
#include <thread>
#include <chrono>
#include <boost/dll.hpp>
#include <boost/dll/library_info.hpp>
#include "ecsact/cli/report.hh"
#include "ecsact/runtime/dylib.h"

#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#endif

namespace fs = std::filesystem;

static auto maybe_library_info( //
	std::filesystem::path library_path
) -> std::unique_ptr<boost::dll::library_info> {
	try {
		return std::unique_ptr<boost::dll::library_info>(
			new boost::dll::library_info{
				library_path.string(),
				false,
			}
		);
	} catch(const std::exception&) {
		return nullptr;
	}
}

static auto check_library_info(
	fs::path                    library_path,
	const ecsact::build_recipe& recipe
) -> bool {
	auto runtime_lib_info = maybe_library_info(library_path);

	if(!runtime_lib_info) {
		ecsact::cli::report_error(
			"Unable to load library info for {}",
			library_path.string()
		);
		return false;
	}

	auto recipe_exports = recipe.exports();
	auto recipe_imports = recipe.imports();

	auto missing_export_symbols = std::vector<std::string>{};
	missing_export_symbols.reserve(recipe_exports.size());

	for(auto recipe_export : recipe_exports) {
		auto has_export_symbol = false;
		for(auto symbol : runtime_lib_info->symbols()) {
			if(recipe_export == symbol) {
				has_export_symbol = true;
			}
		}

		if(!has_export_symbol) {
			missing_export_symbols.push_back(recipe_export);
		}
	}

	for(auto export_symbol : missing_export_symbols) {
		ecsact::cli::report_error("Missing export symbol '{}'", export_symbol);
	}

	if(!missing_export_symbols.empty()) {
		auto found_symbols = runtime_lib_info->symbols();
		if(found_symbols.empty()) {
			ecsact::cli::report_warning("No symbols found");
		} else {
			for(auto symbol : runtime_lib_info->symbols()) {
				ecsact::cli::report_info("Found symbol: {}", symbol);
			}
		}

		return false;
	}

	auto has_dylib_set_fn_symbol = false;

	if(!recipe_imports.empty()) {
		for(auto symbol : runtime_lib_info->symbols()) {
			if(symbol == "ecsact_dylib_set_fn_addr") {
				has_dylib_set_fn_symbol = true;
				break;
			}
		}

		if(!has_dylib_set_fn_symbol) {
			ecsact::cli::report_error(
				"Build recipes with imports must export 'ecsact_dylib_set_fn_addr' "
				"(internal error)"
			);
			return false;
		}
	}

	return true;
}

static auto check_runtime_info(
	fs::path                    library_path,
	const ecsact::build_recipe& recipe
) -> bool {
	auto ec = std::error_code{};

#if defined(_WIN32) && !defined(ECSACT_CLI_USE_SDK_VERSION)
	auto cookie =
		AddDllDirectory(fs::absolute(library_path).parent_path().c_str());

	auto profile_lib = boost::dll::shared_library{
		((library_path.parent_path() / "profiler.dll").c_str()),
		boost::dll::load_mode::default_mode,
		ec,
	};

	RemoveDllDirectory(cookie);
#endif

	auto runtime_lib = boost::dll::shared_library{
		library_path.string(),
		boost::dll::load_mode::default_mode,
		ec,
	};

	if(ec) {
		ecsact::cli::report_error(
#ifdef _WIN32
			"Unable to load {}: {}",
			library_path.string(),
			ec.message()
#else
			"{}",
			dlerror()
#endif
		);
		ecsact::cli::report_error(
			"Cannot verify recipe imports if runtime does not load"
		);
		return false;
	}

	if(runtime_lib.has("ecsact_dylib_has_fn")) {
		decltype(&ecsact_dylib_has_fn) dylib_has_fn = nullptr;
		dylib_has_fn =
			runtime_lib.get<decltype(ecsact_dylib_has_fn)>("ecsact_dylib_has_fn");

		for(auto imp : recipe.imports()) {
			if(!dylib_has_fn(imp.c_str())) {
				ecsact::cli::report_error(
					"ecsact_dylib_has_fn(\"{0}\") returned false",
					imp
				);
				return false;
			}
		}
	}

	runtime_lib.unload();
	return true;
}

auto ecsact::cli::taste_recipe( //
	const ecsact::build_recipe& recipe,
	std::filesystem::path       output_path
) -> int {
	auto ec = std::error_code{};

	if(!fs::exists(output_path)) {
		ecsact::cli::report_error(
			"Output path {} does not exist",
			output_path.string()
		);
		return 1;
	}

	auto library_info = check_library_info(output_path, recipe);
	if(!library_info) {
		return 1;
	}

	auto runtime_info = check_runtime_info(output_path, recipe);
	if(!runtime_info) {
		return 1;
	}

	return 0;
}
