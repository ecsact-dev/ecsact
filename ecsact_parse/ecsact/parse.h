#ifndef ECSACT_PARSER_H
#define ECSACT_PARSER_H

#include "ecsact/parse/statements.h"
#include "ecsact/parse/status.h"
#include "ecsact/parse/error.h"

#ifndef ECSACT_PARSE_API
#	ifdef __cplusplus
#		define ECSACT_PARSE_API extern "C"
#	else
#		define ECSACT_PARSE_API extern
#	endif
#endif // ECSACT_PARSE_API

/**
 * Reads a single statement outputting the parse status and the resulting
 * statement.
 * @param statement_string the string that is parsed
 * @param max_read_length the length of `statement_string`
 * @param context_statement contextual statement that should be considered while
 *        parsing `statement_string`. May be `NULL`.
 * @returns read length
 */
ECSACT_PARSE_API int32_t ecsact_parse_statement(
	const char*             statement_string,
	int                     max_read_length,
	const ecsact_statement* context_statement,
	ecsact_statement*       out_statement,
	ecsact_parse_status*    out_status
);

#endif // ECSACT_PARSER_H
