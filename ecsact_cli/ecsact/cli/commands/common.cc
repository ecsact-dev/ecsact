#include "ecsact/cli/commands/common.hh"

#include <iostream>
#include "docopt.h"
#include "ecsact/cli/report.hh"
#include "ecsact/cli/detail/json_report.hh"
#include "ecsact/cli/detail/text_report.hh"

auto ecsact::cli::detail::process_common_args( //
	const docopt::Options& args
) -> int {
	auto format = args.at("--format").asString();
	auto uses_stdout = args.contains("--stdout") && args.at("--stdout").asBool();

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

	auto report_filter = args.at("--report_filter").asString();
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
