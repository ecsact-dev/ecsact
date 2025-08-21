#include "ecsact/cli/commands/codegen/codegen_util.hh"

#include <unordered_set>
#include <format>
#include <boost/dll.hpp>
#include "ecsact/cli/report.hh"
#include "ecsact/cli/detail/executable_path/executable_path.hh"

namespace fs = std::filesystem;
using namespace std::string_literals;

static auto is_maybe_default_plugin(std::string_view plugin_arg) -> bool {
	const bool is_maybe_named_plugin = plugin_arg.find('/') ==
			std::string::npos &&
		plugin_arg.find('\\') == std::string::npos &&
		plugin_arg.find('.') == std::string::npos;
	return is_maybe_named_plugin;
}

auto ecsact::cli::get_default_plugins_dir() -> fs::path {
	using executable_path::executable_path;

	return fs::weakly_canonical(
		executable_path().parent_path() / ".." / "share" / "ecsact" / "plugins"
	);
}

auto ecsact::cli::is_default_plugin(std::string_view plugin_arg) -> bool {
	if(is_maybe_default_plugin(plugin_arg)) {
		auto default_plugin_path =
			get_default_plugins_dir() / std::format("ecsact_{}_codegen", plugin_arg);
		default_plugin_path.replace_extension(
			current_platform_codegen_plugin_extension()
		);
		return fs::exists(default_plugin_path);
	}
	return false;
}

auto ecsact::cli::current_platform_codegen_plugin_extension() -> std::string {
	return boost::dll::shared_library::suffix().string();
}

auto ecsact::cli::resolve_plugin_path(
	const resolve_plugin_path_options& options,
	std::vector<fs::path>&             checked_plugin_paths
) -> std::optional<fs::path> {
	const bool is_maybe_named_plugin =
		is_maybe_default_plugin(options.plugin_arg);

	if(is_maybe_named_plugin) {
		fs::path& default_plugin_path = checked_plugin_paths.emplace_back(
			options.default_plugins_dir /
			("ecsact_"s + options.plugin_arg + "_codegen"s)
		);

		default_plugin_path.replace_extension(
			current_platform_codegen_plugin_extension()
		);

		if(fs::exists(default_plugin_path)) {
			return default_plugin_path;
		}
	}

	fs::path plugin_path = fs::path(options.plugin_arg).lexically_normal();
	if(plugin_path.extension().empty()) {
		plugin_path.replace_extension(current_platform_codegen_plugin_extension());
	}

	if(fs::exists(plugin_path)) {
		return plugin_path;
	}

	if(plugin_path.is_relative()) {
		for(auto dir : options.additional_plugin_dirs) {
			checked_plugin_paths.emplace_back(dir / plugin_path);
			if(fs::exists(dir / plugin_path)) {
				return dir / plugin_path;
			}
		}
	}

	checked_plugin_paths.emplace_back(plugin_path);

	return {};
}

auto ecsact::cli::resolve_plugin_path( //
	const resolve_plugin_path_options& options
) -> std::optional<fs::path> {
	auto checked_plugin_paths = std::vector<fs::path>{};
	auto plugin_path = resolve_plugin_path(options, checked_plugin_paths);

	if(!plugin_path) {
		auto paths_checked_str = std::string{};

		for(auto& checked_path : checked_plugin_paths) {
			auto checked_path_str = std::string{};
			if(std::getenv("BUILD_WORKSPACE_DIRECTORY") != nullptr) {
				// Easier to follow absolute paths in a bazel run/test environment
				checked_path_str = fs::absolute(checked_path).generic_string();
			} else {
				checked_path_str = fs::relative(checked_path).generic_string();
			}

			paths_checked_str += std::format(" - {}\n", checked_path_str);
		}
		ecsact::cli::report_error(
			"Unable to find codegen plugin '{}'. Paths checked:\n{}",
			options.plugin_arg,
			paths_checked_str
		);

		return std::nullopt;
	}

	return plugin_path;
}
