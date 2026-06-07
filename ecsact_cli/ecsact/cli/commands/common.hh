#pragma once

#include <iostream>
#include <string_view>
#include "ecsact/cli/report.hh"
#include "ecsact/cli/detail/json_report.hh"
#include "ecsact/cli/detail/text_report.hh"

namespace ecsact::cli::detail {

template<typename ParseResult>
auto process_common_args(const ParseResult& args) -> int {
	auto format = args.get_string("--format");
	auto uses_stdout = args.get_bool("--stdout");

	if(format == "text") {
		if(uses_stdout) {
			set_report_handler(text_report_stderr_only{});
		} else {
			set_report_handler(text_report{});
		}
	} else if(format == "json") {
		if(uses_stdout) {
			set_report_handler(json_report_stderr_only{});
		} else {
			set_report_handler(json_report{});
		}
	} else if(format == "none") {
		set_report_handler({});
	} else {
		std::cerr << "Invalid --format value: " << format << "\n";
		return 1;
	}

	auto report_filter = args.get_string("--report_filter");
	if(report_filter == "none") {
		set_report_filter(report_filter::none);
	} else if(report_filter == "error_only") {
		set_report_filter(report_filter::error_only);
	} else if(report_filter == "errors_and_warnings") {
		set_report_filter(report_filter::errors_and_warnings);
	} else {
		std::cerr << "Invalid --report_filter value: " << report_filter << "\n";
		return 1;
	}

	return 0;
}

} // namespace ecsact::cli::detail
