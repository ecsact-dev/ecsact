#pragma once

#include <string>
#include "ecsact/interpret/eval_error.h"

namespace ecsact {

struct parse_eval_error {
	ecsact_eval_error_code eval_error = {};
	int                    source_index = -1;
	int                    line = -1;
	int                    character = -1;
	std::string            error_message;
};

} // namespace ecsact
