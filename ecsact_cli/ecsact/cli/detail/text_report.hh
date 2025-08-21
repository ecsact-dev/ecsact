#pragma once

#include "ecsact/cli/report_message.hh"

namespace ecsact::cli::detail {
struct text_report {
	auto operator()(const message_variant_t&) const -> void;
};

struct text_report_stderr_only {
	auto operator()(const message_variant_t&) const -> void;
};
} // namespace ecsact::cli::detail
