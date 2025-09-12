#pragma once

#include <memory>
#include "tools/cpp/runfiles/runfiles.h"

namespace bazel_sundry {
	using bazel::tools::cpp::runfiles::Runfiles;

	/**
	 * Create bazel runfiles instance without having to specify argv0. This is
	 * done by using platform specific APIs with a "best effort" approach.
	 */
	std::unique_ptr<Runfiles> CreateDefaultRunfiles();
}
