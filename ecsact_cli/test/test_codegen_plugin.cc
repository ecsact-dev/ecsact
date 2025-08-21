#include <string_view>
#include "ecsact/codegen/plugin.h"

using namespace std::string_view_literals;

auto ecsact_codegen_plugin_name() -> const char* {
	return "txt";
}

auto ecsact_codegen_plugin( //
	ecsact_package_id          package_id,
	ecsact_codegen_write_fn_t  write_fn,
	ecsact_codegen_report_fn_t report_fn
) -> void {
	auto test_message = "this is just a test, breathe!"sv;
	write_fn(0, test_message.data(), test_message.size());

	auto example_report_msg = "example warning, don't worry"sv;
	report_fn(
		0,
		ECSACT_CODEGEN_REPORT_WARNING,
		example_report_msg.data(),
		example_report_msg.size()
	);
}
