#include <array>
#include <string>
#include "ecsact/codegen/plugin.h"
#include "ecsact/codegen/plugin.hh"

using namespace std::string_literals;

auto ecsact_codegen_output_filenames( //
	[[maybe_unused]] ecsact_package_id package_id,
	char* const*                       out_filenames,
	int32_t                            max_filenames,
	int32_t                            max_filename_length,
	int32_t*                           out_filenames_length
) -> void {
	ecsact::set_codegen_plugin_output_filenames(
		std::array{
			"test.a"s,
			"test.b"s,
			"test.c"s,
		},
		out_filenames,
		max_filenames,
		max_filename_length,
		out_filenames_length
	);
}

auto ecsact_codegen_plugin_name() -> const char* {
	return "mock";
}

auto ecsact_codegen_plugin( //
	[[maybe_unused]] ecsact_package_id          package_id,
	[[maybe_unused]] ecsact_codegen_write_fn_t  write_fn,
	[[maybe_unused]] ecsact_codegen_report_fn_t report_fn
) -> void {
	auto ctx = ecsact::codegen_plugin_context{package_id, 0, write_fn, report_fn};
	auto world = std::string{"world"};
	ctx.writef("hello {}", world);
}
