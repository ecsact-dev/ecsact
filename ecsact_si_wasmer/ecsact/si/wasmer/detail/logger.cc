#include "ecsact/si/wasmer/detail/logger.hh"

#include <mutex>
#include <map>
#include <string>
#include <string_view>

static auto _logger_mutex = std::recursive_mutex{};
static auto _logger_entries =
	std::vector<ecsact::wasm::detail::log_line_entry>{};
static auto _stdio_strings = [] {
	return std::map<ecsact_si_wasm_log_level, std::string>{
		{ECSACT_SI_WASM_LOG_LEVEL_INFO, {}},
		{ECSACT_SI_WASM_LOG_LEVEL_ERROR, {}},
	};
}();

ecsact::wasm::detail::log_transaction::log_transaction(std::recursive_mutex& m)
	: _lk(m) {
}

ecsact::wasm::detail::log_transaction::log_transaction(log_transaction&& other)
	: _lk(std::move(other._lk)) {
}

ecsact::wasm::detail::log_transaction::~log_transaction() {
}

auto ecsact::wasm::detail::start_transaction() -> log_transaction {
	return log_transaction{_logger_mutex};
}

auto ecsact::wasm::detail::push_log_line(
	const log_transaction&,
	log_line_entry entry
) -> void {
	_logger_entries.emplace_back(std::move(entry));
}

auto ecsact::wasm::detail::get_log_lines(const log_transaction&)
	-> const std::vector<log_line_entry>& {
	return _logger_entries;
}

auto ecsact::wasm::detail::clear_log_lines(const log_transaction&) -> void {
	_logger_entries.clear();
}

auto ecsact::wasm::detail::push_stdio_str(
	ecsact_si_wasm_log_level level,
	std::string_view         str
) -> void {
	std::scoped_lock lk(_logger_mutex);
	_stdio_strings[level] += std::string(str);
}

auto ecsact::wasm::detail::consume_stdio_str_as_log_lines(
	const log_transaction&
) -> std::vector<log_line_entry> {
	using namespace std::string_view_literals;

	auto result = std::vector<log_line_entry>{};

	for(auto&& [log_level, str] : _stdio_strings) {
		if(str.empty()) {
			continue;
		}

		auto idx = 0;
		while(idx != std::string::npos) {
			auto next_idx = str.find('\n', idx + 1);
			auto message = str.substr(idx + 1, next_idx);
			idx = next_idx;
			if(message.empty()) {
				continue;
			}

			result.push_back(
				log_line_entry{
					.log_level = log_level,
					.message = message,
				}
			);
		}
	}

	_stdio_strings.clear();

	return result;
}
