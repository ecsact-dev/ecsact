#include "ecsact/cli/commands/codegen/codegen.hh"

#include <filesystem>
#include <string.h>
#include <array>
#include <format>
#include <boost/dll.hpp>
#include "ecsact/runtime/meta.h"
#include "ecsact/cli/report.hh"
#include "ecsact/codegen/plugin.h"
#include "ecsact/runtime/dylib.h"

namespace fs = std::filesystem;
using namespace std::string_literals;

static std::vector<std::ofstream> file_write_streams;
static bool                       received_fatal_codegen_report = false;

static auto valid_filename_index(int32_t filename_index) -> bool {
	if(filename_index < 0 || filename_index > file_write_streams.size() - 1) {
		received_fatal_codegen_report = true;
		ecsact::cli::report_error(
			"Plugin used invalid filename index. Please contact plugin author."
		);
		return false;
	}

	return true;
}

static void file_write_fn(
	int32_t     filename_index,
	const char* str,
	int32_t     str_len
) {
	if(!valid_filename_index(filename_index)) {
		return;
	}
	file_write_streams.at(filename_index) << std::string_view(str, str_len);
}

static auto codegen_report_fn(
	int32_t                            filename_index,
	ecsact_codegen_report_message_type type,
	const char*                        str,
	int32_t                            str_len
) -> void {
	if(!valid_filename_index(filename_index)) {
		return;
	}
	auto msg = std::string{str, static_cast<size_t>(str_len)};
	switch(type) {
		default:
		case ECSACT_CODEGEN_REPORT_INFO:
			return report(ecsact::cli::info_message{msg});
		case ECSACT_CODEGEN_REPORT_WARNING:
			return report(ecsact::cli::warning_message{msg});
		case ECSACT_CODEGEN_REPORT_ERROR:
			return report(ecsact::cli::error_message{msg});
		case ECSACT_CODEGEN_REPORT_FATAL:
			received_fatal_codegen_report = true;
			return report(ecsact::cli::error_message{msg});
	}
}

auto ecsact::cli::codegen(codegen_options options) -> int {
	auto plugins = std::vector<boost::dll::shared_library>{};
	// key = plugin name, value = plugin path
	auto plugin_names = std::unordered_map<std::string, std::string>{};

	auto unload_plugins = [&plugins] {
		for(auto& plugin : plugins) {
			if(plugin) {
				plugin.unload();
			}
		}
	};

	for(auto plugin_path : options.plugin_paths) {
		auto  ec = std::error_code{};
		auto& plugin = plugins.emplace_back();
		plugin.load(plugin_path.string(), ec);
		if(ec) {
			ecsact::cli::report_error(
				"Failed to load plugin {}: {}",
				plugin_path.string(),
				ec.message()
			);
			unload_plugins();
			return 1;
		}

		auto plugin_name_fn = plugin.get<decltype(ecsact_codegen_plugin_name)>(
			"ecsact_codegen_plugin_name"
		);
		std::string plugin_name = plugin_name_fn();
		if(plugin_names.contains(plugin_name)) {
			ecsact::cli::report_error("Multiple plugins with name '{}'", plugin_name);
			unload_plugins();
			return 1;
		}
	}

	std::vector<ecsact_package_id> package_ids;
	package_ids.resize(static_cast<int32_t>(ecsact_meta_count_packages()));
	ecsact_meta_get_package_ids(
		static_cast<int32_t>(package_ids.size()),
		package_ids.data(),
		nullptr
	);

	// key is output path, value is plugin responsible for generating it
	auto output_paths = std::unordered_map<std::string, std::string>{};
	auto has_plugin_error = false;

	for(auto& plugin : plugins) {
		// precondition: these methods should've been checked in validation
		assert(plugin.has("ecsact_codegen_plugin"));
		assert(plugin.has("ecsact_codegen_plugin_name"));
		assert(plugin.has("ecsact_dylib_set_fn_addr"));

		auto plugin_fn =
			plugin.get<decltype(ecsact_codegen_plugin)>("ecsact_codegen_plugin");
		auto plugin_name_fn = plugin.get<decltype(ecsact_codegen_plugin_name)>(
			"ecsact_codegen_plugin_name"
		);
		std::string plugin_name = plugin_name_fn();

		decltype(&ecsact_dylib_has_fn) dylib_has_fn = nullptr;

		if(plugin.has("ecsact_dylib_has_fn")) {
			dylib_has_fn =
				plugin.get<decltype(ecsact_dylib_has_fn)>("ecsact_dylib_has_fn");
		}

		auto dylib_set_fn_addr =
			plugin.get<decltype(ecsact_dylib_set_fn_addr)>("ecsact_dylib_set_fn_addr"
			);

		auto set_meta_fn_ptr = [&](const char* fn_name, auto fn_ptr) {
			if(dylib_has_fn && !dylib_has_fn(fn_name)) {
				return;
			}
			dylib_set_fn_addr(fn_name, reinterpret_cast<void (*)()>(fn_ptr));
		};

#define CALL_SET_META_FN_PTR(fn_name, unused) \
	set_meta_fn_ptr(#fn_name, &::fn_name)
		FOR_EACH_ECSACT_META_API_FN(CALL_SET_META_FN_PTR);
#undef CALL_SET_META_FN_PTR

		if(options.outdir && !fs::exists(*options.outdir)) {
			auto ec = std::error_code{};
			fs::create_directories(*options.outdir, ec);
			if(ec) {
				ecsact::cli::report_error(
					"Failed to create out directory {}: {}",
					options.outdir->string(),
					ec.message()
				);
				unload_plugins();
				return 3;
			}
		}

		for(auto package_id : package_ids) {
			auto package_file_path =
				fs::path{ecsact_meta_package_file_path(package_id)};

			if(package_file_path.empty()) {
				has_plugin_error = true;
				ecsact::cli::report_error( //
					"Could not find package source file path from "
					"ecsact_meta_package_file_path"
				);
				continue;
			}

			auto plugin_output_paths = std::vector<fs::path>{};
			if(plugin.has("ecsact_codegen_output_filenames")) {
				auto plugin_outputs_fn =
					plugin.get<decltype(ecsact_codegen_output_filenames)>(
						"ecsact_codegen_output_filenames"
					);

				auto filenames_count = int32_t{};
				plugin_outputs_fn(package_id, nullptr, 0, 0, &filenames_count);
				if(filenames_count <= 0) {
					has_plugin_error = true;
					ecsact::cli::report_error(
						"Plugin '{}' ({}) ecsact_codegen_output_filenames returned {} "
						"filenames. Expected 1 or more.",
						plugin_name,
						plugin.location().filename().string(),
						filenames_count
					);
					continue;
				}

				auto filenames = std::array<char*, 16>{};
				auto filenames_ =
					std::array<std::array<char, 1024>, filenames.size()>{};
				for(auto i = 0; filenames_.size() > i; ++i) {
					filenames[i] = filenames_[i].data();
				}

				plugin_outputs_fn(package_id, filenames.data(), 16, 1024, nullptr);

				for(auto i = 0; filenames_count > i; ++i) {
#if defined(__STDC_WANT_SECURE_LIB__) && __STDC_WANT_SECURE_LIB__
					auto filename =
						std::string_view{filenames[i], strnlen_s(filenames[i], 1023)};
#else
					auto filename = std::string_view{filenames[i], strlen(filenames[i])};
#endif
					plugin_output_paths.emplace_back(
						fs::path{package_file_path}.parent_path() / filename
					);
				}
			} else {
				plugin_output_paths.emplace_back(
					fs::path{package_file_path}.replace_extension(
						package_file_path.extension().string() + "." + plugin_name
					)
				);
			}

			if(options.outdir) {
				for(auto& output_file_path : plugin_output_paths) {
					output_file_path = *options.outdir / output_file_path.filename();
				}
			}

			auto has_plugin_output_conflict_error = false;
			for(auto filename_index = 0; plugin_output_paths.size() > filename_index;
					++filename_index) {
				auto output_file_path = plugin_output_paths.at(filename_index);
				if(output_paths.contains(output_file_path.string())) {
					has_plugin_error = true;
					has_plugin_output_conflict_error = true;
					auto conflicting_plugin_name =
						output_paths[output_file_path.string()];

					ecsact::cli::report_error(
						"Plugin '{}' ({}) has conflicts with plugin '{}' output file "
						"{}",
						plugin_name,
						plugin.location().filename().string(),
						conflicting_plugin_name,
						output_file_path.string()
					);
				}
			}

			if(!has_plugin_output_conflict_error) {
				// We're filling this in the for loop. We shouldn't have any in here.
				assert(file_write_streams.empty());

				if(options.only_print_output_files) {
					for(auto& output_file_path : plugin_output_paths) {
						report(output_path_message{
							fs::weakly_canonical(output_file_path).generic_string()
						});
					}
					plugin.unload();
					continue;
				}

				if(options.write_fn) {
					if(plugin_output_paths.size() > 1) {
						// TODO: this error can be misleading if a non-stdout custom
						// write_fn is used
						report_error("cannot use --stdout with multiple output files");
						return 1;
					}

					plugin_fn(package_id, *options.write_fn, &codegen_report_fn);
				} else {
					for(auto filename_index = 0;
							plugin_output_paths.size() > filename_index;
							++filename_index) {
						auto output_file_path = plugin_output_paths.at(filename_index);
						output_paths.emplace(output_file_path.string(), plugin_name);
						if(fs::exists(output_file_path)) {
							fs::permissions(output_file_path, fs::perms::all);
						}

						auto& file_write_stream = file_write_streams.emplace_back();
						file_write_stream.open(output_file_path);
					}

					plugin_fn(package_id, &file_write_fn, &codegen_report_fn);
				}

				if(received_fatal_codegen_report) {
					received_fatal_codegen_report = false;
					has_plugin_error = true;
					report_error("Codegen plugin '{}' reported fatal error", plugin_name);
				}

				for(auto& file_write_stream : file_write_streams) {
					file_write_stream.flush();
					file_write_stream.close();
				}
				file_write_streams.clear();
			}
		}
		plugin.unload();
	}
	if(has_plugin_error) {
		return 2;
	}

	return 0;
}
