#pragma once

#include <string>
#include <mutex>
#include <vector>

#include "ecsact/si/wasm.h"

namespace ecsact::wasm::detail {

struct log_line_entry {
	ecsact_si_wasm_log_level log_level = {};
	std::string              message;
};

class log_transaction {
	std::unique_lock<std::recursive_mutex> _lk;
	log_transaction(std::recursive_mutex& m);
	friend auto start_transaction() -> log_transaction;

public:
	log_transaction(log_transaction&&);
	~log_transaction();
};

auto start_transaction() -> log_transaction;

auto push_log_line(const log_transaction& transaction, log_line_entry entry)
	-> void;

auto get_log_lines(const log_transaction& transaction)
	-> const std::vector<log_line_entry>&;

auto clear_log_lines(const log_transaction& transaction) -> void;

// Safely push string from stdio to a queue that will be consumed later in
// proper log lines.
auto push_stdio_str(ecsact_si_wasm_log_level level, std::string_view str)
	-> void;

auto consume_stdio_str_as_log_lines(const log_transaction& transaction)
	-> std::vector<log_line_entry>;

} // namespace ecsact::wasm::detail
