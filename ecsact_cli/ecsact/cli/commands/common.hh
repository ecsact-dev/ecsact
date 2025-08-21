#pragma once

#include "docopt.h"

namespace ecsact::cli::detail {
auto process_common_args(const docopt::Options& args) -> int;
} // namespace ecsact::cli::detail
