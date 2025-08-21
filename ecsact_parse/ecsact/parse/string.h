#ifndef ECSACT_PARSE_STATEMENT_SV_H
#define ECSACT_PARSE_STATEMENT_SV_H

/**
 * Represents a string view
 */
typedef struct {
	const char* data;
	int         length;
} ecsact_statement_sv;

/**
 * Takes the `src` data and length and derives a copy onto `dst`. This assumes
 * the full data pointer points to a similarly formatted string thus the 'mock'
 * part of the function name. No memory allocation is done in this function.
 */
inline void ecsact_statement_sv_mock_copy(
	const char*                src_full_data,
	const ecsact_statement_sv* src,
	const char*                dst_full_data,
	ecsact_statement_sv*       dst
) {
	auto offset = src->data - src_full_data;
	dst->data = dst_full_data + offset;
	dst->length = src->length;
}

#endif // ECSACT_PARSE_STATEMENT_SV_H
