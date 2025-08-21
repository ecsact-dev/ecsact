#include "ecsact/cli/commands/recipe-bundle/build_recipe_bundle.hh"

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cerrno>
#include <span>
#include <ratio>
#include <cstddef>
#ifdef __cpp_lib_stacktrace
#	include <stacktrace>
#endif
#include "magic_enum.hpp"
#include <boost/url.hpp>
#include <archive.h>
#include <archive_entry.h>
#include "xxhash.h"
#include "ecsact/cli/report.hh"
#include "ecsact/cli/detail/download.hh"
#include "ecsact/cli/detail/long_path_workaround.hh"
#include "ecsact/cli/commands/codegen/codegen_util.hh"

using ecsact::cli::detail::long_path_workaround;

using namespace std::string_literals;
using namespace std::string_view_literals;
namespace fs = std::filesystem;

template<class... Ts>
struct overloaded : Ts... {
	using Ts::operator()...;
};
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

template<typename T>
class uninitialized {
	T _val;

public:
	uninitialized() {
	}

	operator T() const {
		return _val;
	}
};

constexpr auto valid_bundle_entry_prefixes = std::array{
	"files/"sv,
	"codegen/"sv,
};

constexpr auto valid_bundle_paths = std::array{
	"ecsact-build-recipe.yml"sv,
};

static auto archive_error_as_logic_error(archive* a) -> std::logic_error {
	auto msg = std::string{archive_error_string(a)};

#ifdef __cpp_lib_stacktrace
	msg += "\n";
	msg += std::to_string(std::stacktrace::current(1));
#endif

	return std::logic_error{msg};
}

static auto is_valid_bundle_entry_path(std::string_view path) -> bool {
	auto valid_entry_prefix_itr =
		std::ranges::find_if(valid_bundle_entry_prefixes, [&](auto& valid_prefix) {
			return path.starts_with(valid_prefix);
		});

	if(valid_entry_prefix_itr != valid_bundle_entry_prefixes.end()) {
		return true;
	}

	auto valid_bundle_path_itr =
		std::ranges::find_if(valid_bundle_paths, [&](auto& valid_bundle_path) {
			return valid_bundle_path == path;
		});

	return valid_bundle_path_itr != valid_bundle_paths.end();
}

static auto read_file(fs::path path) -> std::optional<std::vector<std::byte>> {
	auto ec = std::error_code{};
	auto file_size = fs::file_size(long_path_workaround(path), ec);
	if(ec) {
		ecsact::cli::report_error(
			"failed to read file size {}: {}",
			path.generic_string(),
			ec.message()
		);
		return {};
	}
	auto file = std::ifstream{long_path_workaround(path), std::ios_base::binary};
	if(!file) {
		ecsact::cli::report_error(
			"failed to open file file for reading {}",
			path.generic_string()
		);
		return {};
	}

	auto file_buffer =
		std::vector<std::byte>{file_size, uninitialized<std::byte>{}};
	file.read(reinterpret_cast<char*>(file_buffer.data()), file_size);

	if(!file) {
		ecsact::cli::report_error("failed to read file {}", path.generic_string());
		return {};
	}

	return file_buffer;
}

static auto write_file(fs::path path, std::span<std::byte> data) -> bool {
	if(path.has_parent_path() && !fs::exists(path.parent_path())) {
		auto ec = std::error_code{};
		fs::create_directories(long_path_workaround(path.parent_path()), ec);
		if(ec) {
			ecsact::cli::report_error(
				"failed to create directory {}: {}",
				path.generic_string(),
				ec.message()
			);
			return false;
		}
	}

	auto file = std::ofstream{
		long_path_workaround(path),
		std::ios::binary | std::ios::trunc
	};
	if(!file) {
		ecsact::cli::report_error(
			"failed to open file {}: {}",
			path.generic_string(),
			std::strerror(errno)
		);
		return false;
	}

	file.write(reinterpret_cast<const char*>(data.data()), data.size());

	if(!file) {
		ecsact::cli::report_error(
			"failed to write file {}: {}",
			path.generic_string(),
			std::strerror(errno)
		);
		return false;
	}

	file.flush();
	return true;
}

ecsact::build_recipe_bundle::build_recipe_bundle() = default;
ecsact::build_recipe_bundle::build_recipe_bundle(build_recipe_bundle&&) =
	default;
ecsact::build_recipe_bundle::~build_recipe_bundle() = default;

auto ecsact::build_recipe_bundle::create( //
	const build_recipe& recipe
) -> create_result {
	auto recipe_bundle_bytes = std::vector<std::byte>{};
	auto new_recipe_sources = std::vector<build_recipe::source>{};
	new_recipe_sources.reserve(recipe.sources().size());
	auto a = std::unique_ptr<archive, decltype(&archive_write_free)>{
		archive_write_new(),
		archive_write_free,
	};

	auto write_callback =
		[](archive* a, void* userdata, const void* buffer, size_t length)
		-> la_ssize_t {
		auto& out_recipe_bundle_bytes =
			*static_cast<decltype(&recipe_bundle_bytes)>(userdata);
		out_recipe_bundle_bytes.insert(
			out_recipe_bundle_bytes.end(),
			static_cast<const std::byte*>(buffer),
			static_cast<const std::byte*>(buffer) + length
		);
		return length;
	};

	archive_write_add_filter_lzma(a.get());
	archive_write_set_format_pax_restricted(a.get());
	archive_write_open2(
		a.get(),
		&recipe_bundle_bytes,
		nullptr,
		static_cast<archive_write_callback*>(write_callback),
		nullptr,
		nullptr
	);

	auto entry = archive_entry_new2(a.get());

	auto add_simple_buffer =
		[&](const std::string& path, std::span<const std::byte> buffer) {
			if(buffer.empty()) {
				ecsact::cli::report_warning("{} is empty", path);
			}
			archive_entry_clear(entry);
			archive_entry_set_pathname(entry, path.c_str());
			archive_entry_set_size(entry, buffer.size());
			archive_entry_set_filetype(entry, AE_IFREG);
			archive_write_header(a.get(), entry);
			archive_write_data(a.get(), buffer.data(), buffer.size());
		};

	using source_visitor_result_t =
		std::variant<build_recipe::source, std::logic_error>;

	auto source_visitor = overloaded{
		[&](build_recipe::source_fetch src) -> source_visitor_result_t {
			auto src_url = boost::url{src.url};
			auto src_path_basename = fs::path{src_url.path().c_str()}.filename();
			auto archive_rel_path =
				(fs::path{"files"} / src.outdir.value_or(".") / src_path_basename)
					.lexically_normal();
			auto download_result = ecsact::cli::detail::download_file(src.url);
			if(!download_result) {
				return std::logic_error{std::format(
					"Failed to download {}: {}",
					src.url,
					download_result.error().what()
				)};
			}

			auto file_buffer = *std::move(download_result);

			add_simple_buffer(archive_rel_path.generic_string(), file_buffer);

			return build_recipe::source_path{
				.path = archive_rel_path,
				.outdir = src.outdir,
			};
		},
		[&](build_recipe::source_codegen src) -> source_visitor_result_t {
			auto new_archive_rel_plugin_paths = std::vector<std::string>{};
			new_archive_rel_plugin_paths.reserve(src.plugins.size());
			for(auto plugin : src.plugins) {
				if(cli::is_default_plugin(plugin)) {
					new_archive_rel_plugin_paths.emplace_back(plugin);
					continue;
				}

				auto plugin_file_path =
					cli::resolve_plugin_path(cli::resolve_plugin_path_options{
						.plugin_arg = plugin,
						.default_plugins_dir = recipe.base_directory(),
						.additional_plugin_dirs = {recipe.base_directory()},
					});

				if(!plugin_file_path) {
					return std::logic_error{
						std::format("Unable to resolve codegen plugin: {}", plugin)
					};
				}

				if(plugin_file_path->extension() != ".wasm") {
					ecsact::cli::report_warning(
						"Bundled codegen plugin {} is platform specific. Bundle will only "
						"work on current platform.",
						plugin_file_path->filename().string()
					);
				}

				auto plugin_file_data = read_file(*plugin_file_path);

				if(!plugin_file_data) {
					return std::logic_error{"read file fail"};
				}

				auto archive_rel_path =
					(fs::path{"codegen"} / plugin_file_path->filename())
						.lexically_normal();

				auto& p = new_archive_rel_plugin_paths.emplace_back(
					archive_rel_path.generic_string()
				);
				add_simple_buffer(p, *plugin_file_data);
			}

			src.plugins = new_archive_rel_plugin_paths;

			return src;
		},
		[&](build_recipe::source_path src) -> source_visitor_result_t {
			auto src_path = src.path;
			if(!src_path.is_absolute()) {
				src_path = recipe.base_directory() / src.path;
			}
			auto src_path_basename = src.path.filename();
			auto archive_rel_path =
				(fs::path{"files"} / src.outdir.value_or(".") / src_path_basename)
					.lexically_normal();
			auto file_buffer = read_file(src_path);

			if(!file_buffer) {
				return std::logic_error{"read file fail"};
			}

			add_simple_buffer(archive_rel_path.generic_string(), *file_buffer);

			return build_recipe::source_path{
				.path = archive_rel_path,
				.outdir = src.outdir,
			};
		},
	};

	for(auto& source : recipe.sources()) {
		auto new_source = std::visit(source_visitor, source);
		if(std::holds_alternative<std::logic_error>(new_source)) {
			return std::get<std::logic_error>(new_source);
		}
		new_recipe_sources.emplace_back(
			std::get<build_recipe::source>(std::move(new_source))
		);
	}

	auto new_recipe = recipe.copy();
	new_recipe.update_sources(std::move(new_recipe_sources));

	add_simple_buffer("ecsact-build-recipe.yml", new_recipe.to_yaml_bytes());

	archive_entry_free(entry);
	if(archive_write_close(a.get()) != ARCHIVE_OK) {
		return archive_error_as_logic_error(a.get());
	}

	auto result = build_recipe_bundle{};
	result._bundle_bytes = recipe_bundle_bytes;
	return result;
}

auto ecsact::build_recipe_bundle::from_file( //
	std::filesystem::path bundle_path
) -> create_result {
	auto result = build_recipe_bundle{};
	auto bundle_bytes = read_file(bundle_path);

	if(!bundle_bytes) {
		return std::logic_error{"read bundle fail"};
	}

	result._bundle_bytes = std::move(*bundle_bytes);
	return result;
}

auto ecsact::build_recipe_bundle::bytes() const -> std::span<const std::byte> {
	return std::span{_bundle_bytes.data(), _bundle_bytes.size()};
}

auto ecsact::build_recipe_bundle::extract( //
	std::filesystem::path extract_dir
) const -> extract_result {
	XXH64_hash_t hash = XXH3_64bits(_bundle_bytes.data(), _bundle_bytes.size());
	auto dir = extract_dir / "ecsact-recipe-bundle" / std::format("{}", hash);
	if(!fs::exists(dir)) {
		fs::create_directories(dir);
	}

	auto a = std::unique_ptr<archive, decltype(&archive_read_free)>{
		archive_read_new(),
		archive_read_free,
	};

	auto result = int{};

	result = archive_read_support_filter_all(a.get());
	if(result != ARCHIVE_OK) {
		return archive_error_as_logic_error(a.get());
	}
	result = archive_read_support_format_all(a.get());
	if(result != ARCHIVE_OK) {
		return archive_error_as_logic_error(a.get());
	}
	/* auto test_file = fopen("test.ecsact-recipe-bundle", "r+"); */
	/* result = archive_read_open_FILE(a.get(), test_file); */
	/* if(result != ARCHIVE_OK) { */
	/* 	return archive_error_as_logic_error(a.get()); */
	/* } */
	result = archive_read_open_memory(
		a.get(),
		_bundle_bytes.data(),
		_bundle_bytes.size()
	);
	if(result != ARCHIVE_OK) {
		return archive_error_as_logic_error(a.get());
	}

	auto entry = archive_entry_new2(a.get());
	auto data = std::vector<std::byte>{};

	for(;;) {
		result = archive_read_next_header2(a.get(), entry);
		if(result == ARCHIVE_EOF) {
			break;
		}

		if(result < ARCHIVE_OK) {
			return archive_error_as_logic_error(a.get());
		}

		auto path = //
			fs::path{archive_entry_pathname(entry)} //
				.lexically_normal()
				.generic_string();

		if(!is_valid_bundle_entry_path(path)) {
			return std::logic_error{
				std::format("Invalid path '{}' found in recipe bundle", path)
			};
		}

		auto size = archive_entry_size(entry);

		if(size == 0) {
			return std::logic_error{std::format("No size for {}", path)};
		}

		data.resize(size);
		auto read_size = archive_read_data(a.get(), data.data(), data.size());

		if(read_size != size) {
			return std::logic_error{std::format("Failed to read {}", path)};
		}

		if(!write_file(dir / path, data)) {
			return std::logic_error{
				std::format("Failed to extract {}", (dir / path).generic_string())
			};
		}
	}

	archive_entry_free(entry);

	auto recipe_path = dir / "ecsact-build-recipe.yml";

	if(!fs::exists(recipe_path)) {
		return std::logic_error{
			"Missing required ecsact-build-recipe.yml at root of bundle"
		};
	}

	return recipe_path;
}
