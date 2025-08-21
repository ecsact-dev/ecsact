#ifndef ECSACT_INTERPRET_EVAL_H
#define ECSACT_INTERPRET_EVAL_H

#include <stdint.h>
#include "ecsact/parse/statements.h"
#include "ecsact/runtime/common.h"
#include "ecsact/interpret/eval_error.h"

typedef struct ecsact_eval_error {
	ecsact_eval_error_code code;
	ecsact_statement_sv    relevant_content;
	ecsact_statement_type  context_type;
} ecsact_eval_error;

ecsact_package_id ecsact_eval_package_statement(
	const ecsact_package_statement* package_statement
);

/**
 * Invokes the appropriate ecsact runtime dynamic module methods for the given
 * statement typically from an `ecsact_parse` call.
 * @param package_id the package the statement should be evaluated in
 * @param statement_stack_size size of `statement_stack` parameter
 * @param statement_stack sequential list of size `statement_stack`. The list is
 *        treated like a stack where the last element is the 'top' element. The
 *        'top' element is the statement that is evaluated.
 * @returns error details
 */
ecsact_eval_error ecsact_eval_statement(
	ecsact_package_id       package_id,
	int32_t                 statement_stack_size,
	const ecsact_statement* statement_stack
);

ECSACT_DEPRECATED("unneeded since interpreter does not hold state")
void ecsact_eval_reset();

#endif // ECSACT_INTERPRET_EVAL_H
