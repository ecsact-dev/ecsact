#pragma once

#include <format>
#include <string>
#include "ecsact/cli/report_message.hh"

namespace ecsact::cli {

enum class report_filter {
	none, /// show all report logs
	error_only, // only show error reports
	errors_and_warnings, // only errors and warnings
};

auto report(const message_variant_t& message) -> void;
auto set_report_handler(std::function<void(const message_variant_t&)>) -> void;
auto set_report_filter(report_filter) -> void;

template<typename... Args>
auto report_error(std::format_string<Args...> fmt, Args&&... args) -> void {
	report(
		error_message{
			.content = std::format<Args...>(fmt, std::forward<Args>(args)...),
		}
	);
}

template<typename... Args>
auto report_info(std::format_string<Args...> fmt, Args&&... args) -> void {
	report(
		info_message{
			.content = std::format<Args...>(fmt, std::forward<Args>(args)...),
		}
	);
}

template<typename... Args>
auto report_warning(std::format_string<Args...> fmt, Args&&... args) -> void {
	report(
		warning_message{
			.content = std::format<Args...>(fmt, std::forward<Args>(args)...),
		}
	);
}

template<typename... Args>
auto report_success(std::format_string<Args...> fmt, Args&&... args) -> void {
	report(
		success_message{
			.content = std::format<Args...>(fmt, std::forward<Args>(args)...),
		}
	);
}

template<typename OutputStream>
auto report_stdout(subcommand_id_t id, OutputStream&& output) -> void {
	auto line = std::string{};
	while(output && std::getline(output, line)) {
		report(
			subcommand_stdout_message{
				.id = id,
				.line = line,
			}
		);
	}
}

template<typename OutputStream>
auto report_stderr(subcommand_id_t id, OutputStream&& output) -> void {
	auto line = std::string{};
	while(output && std::getline(output, line)) {
		report(
			subcommand_stderr_message{
				.id = id,
				.line = line,
			}
		);
	}
}

} // namespace ecsact::cli
