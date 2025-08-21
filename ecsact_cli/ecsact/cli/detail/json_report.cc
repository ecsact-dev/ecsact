#include "ecsact/cli/detail/json_report.hh"

#include <iostream>
#include <nlohmann/json.hpp>

namespace ecsact::cli {
// clang-format off
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(alert_message, content)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(info_message, content)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(error_message, content)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ecsact_error_message, ecsact_source_path, message, line, character)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(warning_message, content)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(success_message, content)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(module_methods_message::method_info, method_name, available)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(module_methods_message, module_name, methods)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(subcommand_start_message, id, executable, arguments)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(subcommand_stdout_message, id, line)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(subcommand_stderr_message, id, line)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(subcommand_progress_message, id, description)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(subcommand_end_message, id, exit_code)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(output_path_message, output_path)
// clang-format on
} // namespace ecsact::cli

template<typename MessageT>
void print_json_report(auto&& outstream, const MessageT& message) {
	auto message_json = "{}"_json;
	to_json(message_json, message);
	message_json["type"] = MessageT::type;
	outstream << message_json.dump() + "\n";
}

auto ecsact::cli::detail::json_report::operator()( //
	const message_variant_t& message
) const -> void {
	std::visit(
		[](const auto& message) { print_json_report(std::cout, message); },
		message
	);
}

auto ecsact::cli::detail::json_report_stderr_only::operator()( //
	const message_variant_t& message
) const -> void {
	std::visit(
		[](const auto& message) { print_json_report(std::cerr, message); },
		message
	);
}
