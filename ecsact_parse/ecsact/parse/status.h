#ifndef ECSACT_PARSE_STATUS_H
#define ECSACT_PARSE_STATUS_H

#include "ecsact/parse/string.h"

typedef enum {
	/**
	 * Parse was successful without error
	 */
	ECSACT_PARSE_STATUS_OK,

	/**
	 * Parse was successful without error. Reached start of a block. Next read
	 * statement should use `out_statement` as context. If there was a previous
	 * context then that previous context should be stored until the next
	 * `ECSACT_PARSE_STATUS_BLOCK_END` occurs.
	 */
	ECSACT_PARSE_STATUS_BLOCK_BEGIN,

	/**
	 * Parse was successful without error. Reached end of a block. Context used in
	 * statement read should not be used anymore and the previous context before
	 * the last `ECSACT_PARSE_STATUS_BLOCK_BEGIN` statement was called should be
	 * used.
	 */
	ECSACT_PARSE_STATUS_BLOCK_END,

	/**
	 * Parse was successful, but expected the statement to end. Usually this means
	 * there is a missing semi-colon or open curly bracket.
	 */
	ECSACT_PARSE_STATUS_ASSUMED_STATEMENT_END,

	/**
	 * Failed to parse statement due to unexpected end of file or end of string
	 * input.
	 */
	ECSACT_PARSE_STATUS_UNEXPECTED_EOF,

	/**
	 *
	 */
	ECSACT_PARSE_STATUS_SYNTAX_ERROR,
} ecsact_parse_status_code;

inline bool ecsact_is_error_parse_status_code(ecsact_parse_status_code code) {
	return (int)code > (int)ECSACT_PARSE_STATUS_ASSUMED_STATEMENT_END;
}

struct ecsact_parse_status {
	ecsact_parse_status_code code;

	/**
	 * Character range of where error occurred in statement string
	 */
	ecsact_statement_sv error_location;
};

#endif // ECSACT_PARSE_STATUS_H
