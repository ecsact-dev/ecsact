#pragma once

#include <string>
#include "ecsact/interpret/eval.h"

#include "./fixed_stack.hh"

namespace ecsact::detail {

void check_file_eval_error(
	ecsact_eval_error&      in_out_error,
	ecsact_package_id       package_id,
	ecsact_parse_status     status,
	const ecsact_statement& statement,
	const std::string&      source
);

}
