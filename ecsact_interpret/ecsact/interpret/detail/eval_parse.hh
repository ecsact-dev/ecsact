#pragma once

#include <filesystem>
#include <array>
#include <unordered_set>
#include <cassert>
#include <iostream> //  TODO(ZAUCY): Remove this
#include "magic_enum.hpp"
#include "ecsact/runtime/dynamic.h"
#include "ecsact/runtime/meta.hh"
#include "ecsact/interpret/parse_eval_error.hh"

#include "./file_eval_error.hh"
#include "./fixed_stack.hh"
#include "./stack_util.hh"
#include "./string_util.hh"
#include "./read_util.hh"

template<>
struct magic_enum::customize::enum_range<ecsact_eval_error_code> {
	static constexpr int min = 0;
	static constexpr int max = 2000;
};

namespace ecsact::detail {

constexpr std::array statement_ending_chars{';', '{', '}', '\n'};

template<typename InputStream>
struct statement_reader {
	InputStream                       stream;
	fixed_stack<ecsact_statement, 16> statements;
	fixed_stack<std::string, 16>      sources;
	std::vector<std::string>          next_statement_sources;
	ecsact_statement*                 current_context = nullptr;
	ecsact_parse_status               status = {};
	int                               current_line = 0;
	int                               current_character = 0;

	void reset() {
		current_line = 0;
		current_character = 0;
		status = {};
		current_context = nullptr;
		next_statement_sources.clear();
		statements.clear();
		sources.clear();
	}

	bool can_read_next() {
		return !next_statement_sources.empty() || (stream && !stream.eof());
	}

	void read_next() {
		auto  previous_statement = try_top(statements);
		auto& next_statement = statements.emplace();
		auto& next_source = sources.emplace();

		if(status.code == ECSACT_PARSE_STATUS_ASSUMED_STATEMENT_END) {
			next_source.reserve(1024);
			ecsact::detail::stream_get_until(
				stream,
				next_source,
				statement_ending_chars
			);
			if(!next_statement_sources.empty()) {
				next_source = next_statement_sources.back() + next_source;
				next_statement_sources.pop_back();
			}
			next_source.shrink_to_fit();
		} else if(next_statement_sources.empty()) {
			next_source.reserve(1024);
			ecsact::detail::stream_get_until(
				stream,
				next_source,
				statement_ending_chars
			);
			next_source.shrink_to_fit();
		} else {
			next_source = next_statement_sources.back();
			next_statement_sources.pop_back();
		}

		auto read_data = next_source.data();
		auto read_size = next_source.size();

		current_context = previous_statement ? &previous_statement->get() : nullptr;
		[[maybe_unused]] auto parse_read_amount = ecsact_parse_statement(
			read_data,
			read_size,
			current_context,
			&next_statement,
			&status
		);

		int last_nl_index = -1;
		current_line += count_char(next_source, '\n', last_nl_index);
		if(last_nl_index != -1) {
			current_character = next_source.size() - last_nl_index;
		} else {
			current_character = next_source.size();
		}

		if(!ecsact_is_error_parse_status_code(status.code)) {
			assert(parse_read_amount == next_source.size());
		}
	}

	void pump_status_code() {
		if(status.code == ECSACT_PARSE_STATUS_OK) {
			// We've reached the end of the current statement. Pop it off the stack.
			statements.pop();
			sources.pop();
			current_context = nullptr;
		} else if(status.code == ECSACT_PARSE_STATUS_ASSUMED_STATEMENT_END) {
			// A valid statement was parsed without a statement end being reached. It
			// is expected to not call `pump_status_code` if reading more is required,
			// but this is a "successful" parse. Treated the same as status OK.
			statements.pop();
			sources.pop();
			current_context = nullptr;
		} else if(status.code == ECSACT_PARSE_STATUS_BLOCK_END) {
			// We've reached the end of the current statement and the current block.
			// Pop the current and pop the block.
			statements.pop();
			sources.pop();
			statements.pop();
			sources.pop();
			current_context = nullptr;
		}
	}

	void pop_rewind() {
		auto& last_source = sources.top();
		next_statement_sources.push_back(last_source);
		statements.pop();
		sources.pop();
		current_context = nullptr;
	}

	void pop_discard() {
		statements.pop();
		sources.pop();
		current_context = nullptr;
	}
};

template<typename InputStream>
struct eval_parse_state {
	std::filesystem::path            file_path;
	statement_reader<InputStream>    reader;
	std::optional<ecsact_package_id> package_id;
	bool                             main_package = false;
	std::string                      package_name;
	std::vector<std::string>         imports;
};

template<typename InputStream>
parse_eval_error to_parse_eval_error(
	int32_t                              source_index,
	const statement_reader<InputStream>& reader
) {
	std::string error_message;

	switch(reader.status.code) {
		case ECSACT_PARSE_STATUS_SYNTAX_ERROR:
			error_message = "Failed to parse statement. Syntax error.";
			break;
		case ECSACT_PARSE_STATUS_UNEXPECTED_EOF:
			error_message = "Failed to parse statement. Unexpected EOF.";
			break;
		default:
			error_message = "Unknown parse error.";
			break;
	}

	return parse_eval_error{
		.source_index = source_index,
		.line = reader.current_line,
		.character = reader.current_character,
		.error_message = error_message,
	};
}

template<typename InputStream>
parse_eval_error to_parse_eval_error(
	int32_t                              source_index,
	const statement_reader<InputStream>& reader,
	ecsact_eval_error                    eval_err
) {
	std::string relevant_content(
		eval_err.relevant_content.data,
		eval_err.relevant_content.length
	);
	std::string err_code_str;
	switch(eval_err.code) {
		default:
			err_code_str = magic_enum::enum_name(eval_err.code);
			if(!err_code_str.empty()) {
				err_code_str = magic_enum::enum_name(eval_err.code).substr(16);
			} else {
				assert(false && "Magic Enum failed to get error name");
			}
			break;
	}

	std::string error_message = err_code_str + " (" +
		std::to_string(magic_enum::enum_integer(eval_err.code)) + ")";
	if(!relevant_content.empty()) {
		error_message += " near: " + relevant_content;
	}

	return parse_eval_error{
		.eval_error = eval_err.code,
		.source_index = source_index,
		.line = reader.current_line,
		.character = reader.current_character,
		.error_message = error_message,
	};
}

template<typename InputStream>
void parse_package_statements(
	std::vector<eval_parse_state<InputStream>>& file_states,
	std::vector<parse_eval_error>&              out_errors
) {
	auto source_index = 0;
	for(auto& state : file_states) {
		while(state.reader.can_read_next()) {
			state.reader.read_next();

			if(ecsact_is_error_parse_status_code(state.reader.status.code)) {
				out_errors.push_back(to_parse_eval_error(source_index, state.reader));
				break;
			}

			auto& statement = state.reader.statements.top();

			if(statement.type == ECSACT_STATEMENT_NONE) {
				state.reader.pump_status_code();
				continue;
			}

			if(statement.type != ECSACT_STATEMENT_PACKAGE) {
				out_errors.push_back(parse_eval_error{
					.eval_error = ECSACT_EVAL_ERR_EXPECTED_PACKAGE_STATEMENT,
					.source_index = source_index,
					.line = state.reader.current_line,
					.character = state.reader.current_character,
					.error_message = "Must have package statement as first statement in "
													 "file.",
				});
			} else {
				state.main_package = statement.data.package_statement.main;
				state.package_name = std::string(
					statement.data.package_statement.package_name.data,
					statement.data.package_statement.package_name.length
				);
			}

			state.reader.pump_status_code();
			break;
		}

		source_index += 1;
	}
}

template<typename InputStream>
void parse_imports(
	std::vector<eval_parse_state<InputStream>>& file_states,
	std::vector<parse_eval_error>&              out_errors
) {
	auto source_index = 0;
	for(auto& state : file_states) {
		while(state.reader.can_read_next()) {
			state.reader.read_next();

			if(ecsact_is_error_parse_status_code(state.reader.status.code)) {
				out_errors.push_back(to_parse_eval_error(source_index, state.reader));
			} else {
				ecsact_statement& statement = state.reader.statements.top();
				if(statement.type == ECSACT_STATEMENT_NONE) {
					state.reader.pump_status_code();
					continue;
				}
				if(statement.type != ECSACT_STATEMENT_IMPORT) {
					state.reader.pop_rewind();
					break;
				}
				state.imports.push_back(std::string(
					statement.data.import_statement.import_package_name.data,
					statement.data.import_statement.import_package_name.length
				));

				state.reader.pump_status_code();
			}
		}

		source_index += 1;
	}
}

template<typename InputStream>
void check_unknown_imports(
	std::vector<eval_parse_state<InputStream>>& file_states,
	std::vector<parse_eval_error>&              out_errors
) {
	auto source_index = 0;
	for(auto& state : file_states) {
		for(auto& import_name : state.imports) {
			bool found_import = false;
			for(auto& other_state : file_states) {
				if(state.package_name == other_state.package_name) {
					continue;
				}

				if(import_name == other_state.package_name) {
					found_import = true;
					break;
				}
			}

			if(!found_import) {
				out_errors.push_back({
					.source_index = source_index,
					.line = state.reader.current_line,
					.character = state.reader.current_character,
					.error_message = "Unknown import package '" + import_name + "'",
				});
			}
		}

		source_index += 1;
	}
}

template<typename InputStream>
void check_cyclic_imports(
	std::vector<eval_parse_state<InputStream>>& file_states,
	std::vector<parse_eval_error>&              out_errors
) {
}

template<typename InputStream>
void eval_package_statements(
	std::vector<eval_parse_state<InputStream>>& file_states,
	std::vector<parse_eval_error>&              out_errors
) {
	for(auto& state : file_states) {
		ecsact_package_statement faux_statement{
			.main = state.main_package,
			.package_name{
				.data = state.package_name.c_str(),
				.length = static_cast<int32_t>(state.package_name.size()),
			},
		};
		state.package_id = ecsact_eval_package_statement(&faux_statement);
		auto source_file_path = state.file_path.string();
		ecsact_set_package_source_file_path(
			*state.package_id,
			source_file_path.c_str(),
			static_cast<int32_t>(source_file_path.size())
		);
	}
}

template<typename InputStream>
void eval_imports(
	int32_t                        source_index,
	eval_parse_state<InputStream>& file_state,
	std::vector<parse_eval_error>& out_errors
) {
	for(auto& import_name : file_state.imports) {
		ecsact_statement faux_import_statement{
			.type = ECSACT_STATEMENT_IMPORT,
			.data{.import_statement{
				.import_package_name{
					.data = import_name.c_str(),
					.length = static_cast<int32_t>(import_name.size()),
				},
			}},
		};

		auto eval_err =
			ecsact_eval_statement(*file_state.package_id, 1, &faux_import_statement);

		if(eval_err.code != ECSACT_EVAL_OK) {
			out_errors.push_back(
				to_parse_eval_error(source_index, file_state.reader, eval_err)
			);
		}
	}
}

template<typename InputStream>
void skip_until_current_block_end(statement_reader<InputStream>& reader) {
	assert(reader.status.code == ECSACT_PARSE_STATUS_BLOCK_BEGIN);
	int block_depth = 1;
	while(block_depth > 0) {
		if(!reader.can_read_next()) {
			break;
		}

		reader.read_next();
		if(reader.status.code == ECSACT_PARSE_STATUS_BLOCK_BEGIN) {
			block_depth += 1;
		} else if(reader.status.code == ECSACT_PARSE_STATUS_BLOCK_END) {
			block_depth -= 1;
		}

		reader.pump_status_code();
	}
}

template<typename InputStream>
void parse_eval_declarations(
	int32_t                        source_index,
	eval_parse_state<InputStream>& file_state,
	std::vector<parse_eval_error>& out_errors
) {
	while(file_state.reader.can_read_next()) {
		file_state.reader.read_next();

		if(ecsact_is_error_parse_status_code(file_state.reader.status.code)) {
			out_errors.push_back(to_parse_eval_error(source_index, file_state.reader)
			);
			continue;
		}

		auto eval_err = ecsact_eval_statement(
			*file_state.package_id,
			static_cast<int32_t>(file_state.reader.statements.size()),
			file_state.reader.statements.data()
		);

		if(eval_err.code == ECSACT_EVAL_OK) {
			ecsact::detail::check_file_eval_error(
				eval_err,
				*file_state.package_id,
				file_state.reader.status,
				*file_state.reader.current_context,
				""
			);
		}

		if(eval_err.code == ECSACT_EVAL_OK) {
			file_state.reader.pump_status_code();
		} else {
			out_errors.push_back(
				to_parse_eval_error(source_index, file_state.reader, eval_err)
			);

			if(file_state.reader.status.code == ECSACT_PARSE_STATUS_BLOCK_BEGIN) {
				skip_until_current_block_end(file_state.reader);
			} else {
				file_state.reader.pump_status_code();
			}
		}
	}
}

template<typename InputStream>
inline auto get_sorted_states(
	std::vector<eval_parse_state<InputStream>>& file_states
) -> std::vector<std::reference_wrapper<eval_parse_state<InputStream>>> {
	auto result =
		std::vector<std::reference_wrapper<eval_parse_state<InputStream>>>{};
	result.reserve(file_states.size());

	auto resolved_packages = std::unordered_set<std::string>{};
	resolved_packages.reserve(file_states.size());

	while(resolved_packages.size() != file_states.size()) {
		for(auto& state : file_states) {
			if(resolved_packages.contains(state.package_name)) {
				continue;
			}

			bool imports_resolved = true;
			for(auto import_name : state.imports) {
				if(!resolved_packages.contains(import_name)) {
					imports_resolved = false;
					break;
				}
			}

			if(imports_resolved) {
				resolved_packages.insert(state.package_name);
				result.push_back(std::ref(state));
				break;
			}
		}
	}

	assert(result.capacity() == result.size());
	return result;
}

} // namespace ecsact::detail
