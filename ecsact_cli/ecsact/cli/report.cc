#include "ecsact/cli/report.hh"
#include "report_message.hh"

#include <array>

using ecsact::cli::report_filter;

static auto _report_handler =
	std::function<void(const ecsact::cli::message_variant_t&)>{};

static auto _report_filter = report_filter::none;

template<typename T>
struct report_filter_category {
	static constexpr auto filters = std::array{
		report_filter::none,
	};
};

template<>
struct report_filter_category<ecsact::cli::error_message> {
	static constexpr auto filters = std::array{
		report_filter::none,
		report_filter::error_only,
		report_filter::errors_and_warnings,
	};
};

template<>
struct report_filter_category<ecsact::cli::ecsact_error_message> {
	static constexpr auto filters = std::array{
		report_filter::none,
		report_filter::error_only,
		report_filter::errors_and_warnings,
	};
};

template<>
struct report_filter_category<ecsact::cli::alert_message> {
	static constexpr auto filters = std::array{
		report_filter::none,
		report_filter::error_only,
		report_filter::errors_and_warnings,
	};
};

template<>
struct report_filter_category<ecsact::cli::warning_message> {
	static constexpr auto filters = std::array{
		report_filter::none,
		report_filter::errors_and_warnings,
	};
};

static auto should_report_message( //
	const ecsact::cli::message_variant_t& message
) -> bool {
	return std::visit(
		[](auto&& message) {
			using message_t = std::remove_cvref_t<decltype(message)>;

			for(auto filter : report_filter_category<message_t>::filters) {
				if(_report_filter == filter) {
					return true;
				}
			}

			return false;
		},
		message
	);
}

auto ecsact::cli::report( //
	const message_variant_t& message
) -> void {
	if(_report_handler) {
		if(should_report_message(message)) {
			_report_handler(message);
		}
	}
}

auto ecsact::cli::set_report_handler( //
	std::function<void(const message_variant_t&)> handler
) -> void {
	_report_handler = handler;
}

auto ecsact::cli::set_report_filter(report_filter filter) -> void {
	_report_filter = filter;
}
