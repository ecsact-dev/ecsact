#include <string_view>
#include <array>
#include "ecsact/codegen/plugin.hh"
#include "ecsact/runtime/meta.hh"

using namespace std::string_view_literals;

auto ecsact_codegen_output_filenames( //
	ecsact_package_id package_id,
	char* const*      out_filenames,
	int32_t           max_filenames,
	int32_t           max_filename_length,
	int32_t*          out_filenames_length
) -> void {
	auto pkg_basename = //
		ecsact::meta::package_file_path(package_id)
			.filename()
			.replace_extension("")
			.string();
	ecsact::set_codegen_plugin_output_filenames(
		std::array{
			pkg_basename + ".txt",
			pkg_basename + ".zomsky",
		},
		out_filenames,
		max_filenames,
		max_filename_length,
		out_filenames_length
	);
}

auto ecsact_codegen_plugin_name() -> const char* {
	return "Example Codegen";
}

auto ecsact_codegen_plugin( //
	ecsact_package_id          package_id,
	ecsact_codegen_write_fn_t  write_fn,
	ecsact_codegen_report_fn_t report_fn
) -> void {
	auto test_message = "throw this in the text file"sv;
	write_fn(0, test_message.data(), test_message.size());

	test_message = "throw this in the zomsky file"sv;
	write_fn(1, test_message.data(), test_message.size());
}
