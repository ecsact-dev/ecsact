#include <type_traits>
#include <iostream>
#include <array>
#include <optional>
#include "magic_enum.hpp"
#include "ecsact/runtime/meta.hh"
#include "ecsact/runtime/dynamic.h"
#include "ecsact/parse.h"
#include "ecsact/interpret/eval.h"
#include "ecsact/interpret/detail/eval_parse.hh"

#define COLOR_GREY "\033[90m"
#define COLOR_RED "\033[31m"
#define COLOR_CYAN "\u001b[36m"
#define COLOR_RESET "\033[0m"

bool is_repl_cmd(const std::string& source, std::string cmd_str) {
	if(source == cmd_str) {
		return true;
	}
	if(source == cmd_str + "\n") {
		return true;
	}
	if(source.ends_with("\n" + cmd_str + "\n")) {
		return true;
	}

	return false;
}

int main() {
	// Let's go !!blazingly fast!!
	std::ios_base::sync_with_stdio(false);

	ecsact::detail::statement_reader<std::istream&> reader{std::cin};
	std::optional<ecsact_package_id>                current_package{};

	auto last_line = reader.current_line - 1;

	while(reader.can_read_next()) {
		if(last_line != reader.current_line) {
			const bool not_none_statement = reader.statements.empty() ||
				reader.statements.top().type != ECSACT_STATEMENT_NONE;
			const bool expected_end = reader.status.code ==
				ECSACT_PARSE_STATUS_ASSUMED_STATEMENT_END;

			if(not_none_statement && expected_end) {
				std::cout << COLOR_GREY ".. " COLOR_RESET;
			} else if(reader.statements.empty()) {
				std::cout << "\r" COLOR_GREY " > " COLOR_RESET;
			} else {
				std::cout << COLOR_GREY " | " COLOR_RESET;
			}
		}

		last_line = reader.current_line;

		reader.read_next();

		if(!reader.sources.empty()) {
			const auto& repl_maybe = reader.sources.top();
			if(is_repl_cmd(repl_maybe, ".exit")) {
				return 0;
			} else if(is_repl_cmd(repl_maybe, ".reset")) {
				for(auto pkg_id : ecsact::meta::get_package_ids()) {
					ecsact_destroy_package(pkg_id);
				}
				ecsact_eval_reset();
				reader.reset();
				std::cout << COLOR_GREY " [ reset ]\n" COLOR_RESET;
				continue;
			}
		}

		if(reader.status.code == ECSACT_PARSE_STATUS_ASSUMED_STATEMENT_END) {
			const auto& source = reader.sources.top();
			if(ecsact::detail::is_empty_or_whitespace(source)) {
				reader.pop_discard();
				reader.status = {};
			} else {
				reader.pop_rewind();
			}
		} else if(ecsact_is_error_parse_status_code(reader.status.code)) {
			auto&       last_source = reader.sources.top();
			std::string err_highlight;
			err_highlight.reserve(last_source.size() + 3);
			err_highlight += "   ";
			for(auto c : last_source) {
				if(std::isspace(c)) {
					err_highlight.push_back(c);
				} else {
					err_highlight.push_back('^');
				}
			}

			std::cerr << COLOR_RED << err_highlight << COLOR_RESET " ";

			switch(reader.status.code) {
				case ECSACT_PARSE_STATUS_UNEXPECTED_EOF:
					std::cerr << "Unexpected EOF\n";
					break;
				case ECSACT_PARSE_STATUS_SYNTAX_ERROR:
					std::cerr << "Syntax error\n";
					break;
				default:
					std::cerr << "Unhandled error case\n";
					break;
			}

			reader.pop_discard();
		} else {
			auto& last_statement = reader.statements.top();
			auto& last_source = reader.sources.top();
			if(last_statement.type == ECSACT_STATEMENT_PACKAGE) {
				current_package =
					ecsact_eval_package_statement(&last_statement.data.package_statement);
				reader.pump_status_code();
				std::cout << COLOR_GREY " [ new package " COLOR_CYAN
									<< ecsact::meta::package_name(*current_package)
									<< COLOR_GREY " ]\n" COLOR_RESET;
			} else if(last_statement.type == ECSACT_STATEMENT_UNKNOWN) {
				auto err_highlight = std::string{};
				err_highlight.reserve(last_source.size() + 3);
				err_highlight += "   ";
				for(auto c : last_source) {
					if(std::isspace(c)) {
						err_highlight.push_back(c);
					} else {
						err_highlight.push_back('^');
					}
				}
				std::cerr //
					<< COLOR_RED << err_highlight << COLOR_RESET " "
					<< "Unknown statement.\n";

				reader.pump_status_code();
			} else if(current_package) {
				auto eval_err = ecsact_eval_statement(
					*current_package,
					static_cast<int32_t>(reader.statements.size()),
					reader.statements.data()
				);

				if(eval_err.code == ECSACT_EVAL_OK) {
					reader.pump_status_code();
				} else {
					std::string_view err_content(
						eval_err.relevant_content.data,
						eval_err.relevant_content.length
					);
					int offset = eval_err.relevant_content.data - last_source.c_str();
					if(offset < last_source.size()) {
						std::cerr //
							<< std::string(offset + 3, ' ') << COLOR_RED "^" COLOR_RESET
							<< " " << magic_enum::enum_name(eval_err.code).substr(16)
							<< COLOR_GREY " (" << eval_err.code << ")" COLOR_RESET << "\n";
					} else if(!err_content.empty()) {
						std::cerr << err_content << "\n";
					} else {
						std::cerr //
							<< COLOR_GREY " [ " COLOR_RED "error " COLOR_RESET
							<< magic_enum::enum_name(eval_err.code).substr(16) << COLOR_GREY
							<< " (" << eval_err.code << ") ]" COLOR_RESET "\n";
					}

					reader.pop_discard();
				}
			} else {
				std::string err_highlight;
				err_highlight.reserve(last_source.size() + 3);
				err_highlight += "   ";
				for(auto c : last_source) {
					if(std::isspace(c)) {
						err_highlight.push_back(c);
					} else {
						err_highlight.push_back('^');
					}
				}
				std::cerr //
					<< COLOR_RED << err_highlight << COLOR_RESET " "
					<< "Invalid first statement. Must start with package statement."
					<< "\n\n";
				reader.pop_discard();
			}
		}
	}

	ecsact_eval_reset();

	return 0;
}
