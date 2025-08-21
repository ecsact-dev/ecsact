#pragma once

#include <cassert>
#include <type_traits>
#include <string>
#include <string_view>
#include <cstring>
#include <iterator>
#include <format>
#include <span>
#include <string.h>
#include "ecsact/runtime/common.h"
#include "ecsact/codegen/plugin.h"

namespace ecsact {

/**
 * Helper function to implement the pesky ecsact_codegen_output_filenames C API.
 * @example
 * ```cpp
 * auto ecsact_codegen_output_filenames( //
 *   ecsact_package_id package_id,
 *   char* const*      out_filenames,
 *   int32_t           max_filenames,
 *   int32_t           max_filename_length,
 *   int32_t*          out_filenames_length
 * ) -> void {
 *   auto package_filename =
 *     ecsact::meta::package_file_path(package_id).filename();
 *
 *   // Generate a .h and .cpp file for each package
 *   ecsact::set_codegen_plugin_output_filenames(
 *     {
 *       package_filename.string() + ".h",
 *       package_filename.string() + ".cpp",
 *     },
 *     out_filenames,
 *     max_filenames,
 *     max_filename_length,
 *     out_filenames_length
 *   );
 * }
 * ```
 */
auto set_codegen_plugin_output_filenames(
	const auto&              filenames,
	char* const*             out_filenames,
	int32_t                  max_filenames,
	[[maybe_unused]] int32_t max_filename_length,
	int32_t*                 out_filenames_length
) -> void {
	if(out_filenames != nullptr) {
		for(auto i = 0; max_filenames > i; ++i) {
			if(i >= std::size(filenames)) {
				break;
			}
			auto filename = std::data(filenames) + i;
#if defined(__STDC_WANT_SECURE_LIB__) && __STDC_WANT_SECURE_LIB__
			strcpy_s(out_filenames[i], max_filename_length, filename->c_str());
#else
			strcpy(out_filenames[i], filename->c_str());
#endif
		}
	}

	if(out_filenames_length != nullptr) {
		*out_filenames_length = static_cast<int32_t>(std::size(filenames));
	}
}

/**
 * Helper type to give a more C++ friendly write function
 * @example
 *   void ecsact_codegen_plugin
 *     ( ecsact_package_id           package_id
 *     , ecsact_codegen_write_fn_t   write_fn
 *     , ecsact_codegen_report_fn_t  report_fn
 *     )
 *   {
 *     ecsact::codegen_plugin_context ctx{package_id, write_fn, report_fn};
 *     ctx.writef("Hello, World!\n");
 *     ctx.info("We made it!");
 *   }
 */
struct codegen_plugin_context {
	const ecsact_package_id          package_id;
	const int32_t                    filename_index;
	const ecsact_codegen_write_fn_t  write_fn;
	const ecsact_codegen_report_fn_t report_fn;
	int                              indentation = 0;

	std::string get_indent_str() {
		return std::string(indentation, '\t');
	}

	void report_(
		ecsact_codegen_report_type report_type,
		const char*                str_data,
		int32_t                    str_data_len
	) {
		if(report_fn != nullptr) {
			report_fn(filename_index, report_type, str_data, str_data_len);
		}
	}

	void write_(const char* str_data, int32_t str_data_len) {
		assert(indentation >= 0);

		if(indentation <= 0) {
			write_fn(filename_index, str_data, str_data_len);
		} else {
			std::string_view str(str_data, str_data_len);
			auto             indent_str = get_indent_str();
			auto             nl_idx = str.find('\n');
			while(nl_idx != std::string_view::npos) {
				write_fn(filename_index, str.data(), nl_idx + 1);
				write_fn(filename_index, indent_str.data(), indent_str.size());
				str =
					std::string_view(str.data() + nl_idx + 1, str.size() - nl_idx - 1);
				nl_idx = str.find('\n');
			}

			if(!str.empty()) {
				write_fn(filename_index, str.data(), static_cast<int32_t>(str.size()));
			}
		}
	}

	template<typename T>
	[[deprecated("use writef instead")]]
	void write(T&& arg) {
		using NoRefT = std::remove_cvref_t<T>;

		if constexpr(std::is_same_v<NoRefT, std::string_view>) {
			write_(arg.data(), static_cast<int32_t>(arg.size()));
		} else if constexpr(std::is_same_v<NoRefT, std::string>) {
			write_(arg.data(), static_cast<int32_t>(arg.size()));
		} else if constexpr(std::is_convertible_v<NoRefT, const char*>) {
			write_(arg, std::strlen(arg));
		} else {
			auto str = std::to_string(std::forward<T>(arg));
			write_(str.data(), static_cast<int32_t>(str.size()));
		}
	}

	template<typename... T>
	[[deprecated("use writef instead")]]
	void write(T&&... args) {
		(write<T>(std::forward<T>(args)), ...);
	}

	void write_each(auto&& delim, auto&& range, auto&& callback) {
		auto begin = std::begin(range);
		auto end = std::end(range);
		if(begin != end) {
			callback(*begin);
			for(auto itr = std::next(begin); itr != end; ++itr) {
				write(delim);
				callback(*itr);
			}
		}
	}

	template<typename... Args>
	auto writef(std::format_string<Args...> fmt, Args&&... args) {
		auto str = std::format(fmt, std::forward<Args>(args)...);
		write_(str.data(), static_cast<int32_t>(str.size()));
	}

	template<typename... Args>
	auto info(std::format_string<Args...> fmt, Args&&... args) {
		auto str = std::format(fmt, std::forward<Args>(args)...);
		report_(ECSACT_CODEGEN_REPORT_INFO, str.data(), str.size());
	}

	template<typename... Args>
	auto warn(std::format_string<Args...> fmt, Args&&... args) {
		auto str = std::format(fmt, std::forward<Args>(args)...);
		report_(ECSACT_CODEGEN_REPORT_WARNING, str.data(), str.size());
	}

	template<typename... Args>
	auto error(std::format_string<Args...> fmt, Args&&... args) {
		auto str = std::format(fmt, std::forward<Args>(args)...);
		report_(ECSACT_CODEGEN_REPORT_ERROR, str.data(), str.size());
	}

	template<typename... Args>
	auto fatal(std::format_string<Args...> fmt, Args&&... args) {
		auto str = std::format(fmt, std::forward<Args>(args)...);
		report_(ECSACT_CODEGEN_REPORT_FATAL, str.data(), str.size());
	}
};

} // namespace ecsact
