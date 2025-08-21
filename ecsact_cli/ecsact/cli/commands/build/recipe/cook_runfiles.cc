#include "cook_runfiles.hh"

#include <filesystem>
#include <vector>
#include <string>
#include <system_error>
#include <string_view>

#include "tools/cpp/runfiles/runfiles.h"
#include "ecsact/cli/report.hh"

namespace fs = std::filesystem;
using bazel::tools::cpp::runfiles::Runfiles;
using namespace std::string_view_literals;

auto ecsact::cli::cook::load_runfiles(
	const char*           argv0,
	std::filesystem::path inc_dir
) -> bool {
	auto ec = std::error_code{};

	auto runfiles_error = std::string{};
	auto runfiles = std::unique_ptr<Runfiles>(
		Runfiles::Create(argv0, BAZEL_CURRENT_REPOSITORY, &runfiles_error)
	);
	if(!runfiles) {
		ecsact::cli::report_error("Failed to load runfiles: {}", runfiles_error);
		return false;
	}

	ecsact::cli::report_info("Using ecsact headers from runfiles");

	auto ecsact_runtime_headers_from_runfiles = std::vector<std::string>{
		"ecsact_runtime/ecsact/lib.hh",
		"ecsact_runtime/ecsact/runtime.h",
		"ecsact_runtime/ecsact/runtime/async.h",
		"ecsact_runtime/ecsact/runtime/async.hh",
		"ecsact_runtime/ecsact/runtime/common.h",
		"ecsact_runtime/ecsact/runtime/core.h",
		"ecsact_runtime/ecsact/runtime/core.hh",
		"ecsact_runtime/ecsact/runtime/definitions.h",
		"ecsact_runtime/ecsact/runtime/dylib.h",
		"ecsact_runtime/ecsact/runtime/dynamic.h",
		"ecsact_runtime/ecsact/runtime/meta.h",
		"ecsact_runtime/ecsact/runtime/meta.hh",
		"ecsact_runtime/ecsact/runtime/serialize.h",
		"ecsact_runtime/ecsact/runtime/serialize.hh",
		"ecsact_runtime/ecsact/runtime/static.h",
		"ecsact_runtime/ecsact/si/wasm.h",
		"ecsact_runtime/ecsact/si/wasm.hh",
	};

	for(auto hdr : ecsact_runtime_headers_from_runfiles) {
		auto full_hdr_path = runfiles->Rlocation(hdr);

		if(full_hdr_path.empty()) {
			ecsact::cli::report_error(
				"Cannot find ecsact_runtime header in runfiles: {}",
				hdr
			);
			return false;
		}

		auto rel_hdr_path = hdr.substr("ecsact_runtime/"sv.size());
		fs::create_directories((inc_dir / rel_hdr_path).parent_path(), ec);
		auto some_bullshit = inc_dir / rel_hdr_path;

		if(fs::exists(inc_dir / rel_hdr_path)) {
			ecsact::cli::report_warning(
				"Overwriting ecsact runtime header {} with runfiles header",
				rel_hdr_path
			);
		}

		fs::copy_file(
			full_hdr_path,
			inc_dir / rel_hdr_path,
			fs::copy_options::overwrite_existing,
			ec
		);
		if(ec) {
			ecsact::cli::report_error(
				"Failed to copy ecsact runtime header from runfiles. {} -> {}\n{}",
				full_hdr_path,
				(inc_dir / rel_hdr_path).generic_string(),
				ec.message()
			);
			return false;
		}
	}

	return true;
}

auto ecsact::cli::cook::load_tracy_runfiles(
	const char*           argv0,
	std::filesystem::path tracy_dir
) -> bool {
	auto ec = std::error_code{};

	fs::create_directories(tracy_dir, ec);

	if(ec) {
		ecsact::cli::report_error(
			"Failed to create tracy directory for runfiles. {} -> {}\n",
			tracy_dir.generic_string(),
			ec.message()
		);
		return false;
	}

	auto tracy_src_runfiles = std::vector<std::string>{
		"public/TracyClient.cpp",
		"public/tracy/TracyC.h",
		"public/tracy/Tracy.hpp",
		"public/tracy/TracyD3D11.hpp",
		"public/tracy/TracyD3D12.hpp",
		"public/tracy/TracyLua.hpp",
		"public/tracy/TracyOpenCL.hpp",
		"public/tracy/TracyOpenGL.hpp",
		"public/tracy/TracyVulkan.hpp",
		"public/common/TracyColor.hpp",
		"public/common/TracyAlign.hpp",
		"public/common/TracyAlloc.hpp",
		"public/common/TracyApi.h",
		"public/common/TracyForceInline.hpp",
		"public/common/TracyMutex.hpp",
		"public/common/TracyProtocol.hpp",
		"public/common/TracyQueue.hpp",
		"public/common/TracySocket.hpp",
		"public/common/TracyStackFrames.hpp",
		"public/common/TracySystem.hpp",
		"public/common/TracyUwp.hpp",
		"public/common/TracyVersion.hpp",
		"public/common/TracyYield.hpp",
		"public/common/tracy_lz4.hpp",
		"public/common/tracy_lz4hc.hpp",
		"public/common/TracySystem.cpp",
		"public/common/tracy_lz4.cpp",
		"public/common/TracySocket.cpp",
		"public/client/TracyArmCpuTable.hpp",
		"public/client/TracyCallstack.h",
		"public/client/TracyCallstack.hpp",
		"public/client/TracyCpuid.hpp",
		"public/client/TracyDebug.hpp",
		"public/client/TracyDxt1.hpp",
		"public/client/TracyFastVector.hpp",
		"public/client/TracyKCore.hpp",
		"public/client/TracyLock.hpp",
		"public/client/TracyProfiler.hpp",
		"public/client/TracyRingBuffer.hpp",
		"public/client/TracyScoped.hpp",
		"public/client/TracyStringHelpers.hpp",
		"public/client/TracySysPower.hpp",
		"public/client/TracySysTime.hpp",
		"public/client/TracySysTrace.hpp",
		"public/client/TracyThread.hpp",
		"public/client/tracy_SPSCQueue.h",
		"public/client/tracy_concurrentqueue.h",
		"public/client/tracy_rpmalloc.hpp",
		"public/client/TracyAlloc.cpp",
		"public/client/TracyCallstack.cpp",
		"public/client/TracyDxt1.cpp",
		"public/client/TracyKCore.cpp",
		"public/client/TracyOverride.cpp",
		"public/client/TracyProfiler.cpp",
		"public/client/TracySysPower.cpp",
		"public/client/TracySysTime.cpp",
		"public/client/TracySysTrace.cpp",
		"public/client/tracy_rpmalloc.cpp",
	};

	auto runfiles_error = std::string{};
	auto runfiles = std::unique_ptr<Runfiles>(
		Runfiles::Create(argv0, BAZEL_CURRENT_REPOSITORY, &runfiles_error)
	);
	if(!runfiles) {
		ecsact::cli::report_error("Failed to load runfiles: {}", runfiles_error);
		return false;
	}

	ecsact::cli::report_info("Tracy set, using runfiles");

	for(auto hdr : tracy_src_runfiles) {
		auto full_hdr_path = runfiles->Rlocation("tracy/" + hdr);

		if(full_hdr_path.empty()) {
			ecsact::cli::report_error(
				"Cannot find tracy header in runfiles: {}",
				hdr
			);
			return false;
		}

		auto rel_hdr_path = hdr.substr("public/"sv.size());

		fs::create_directories((tracy_dir / rel_hdr_path).parent_path(), ec);
		auto some_bullshit = tracy_dir / rel_hdr_path;

		if(fs::exists(tracy_dir / rel_hdr_path)) {
			ecsact::cli::report_warning(
				"Overwriting tracy header {} with runfiles header",
				rel_hdr_path
			);
		}

		fs::copy_file(
			full_hdr_path,
			tracy_dir / rel_hdr_path,
			fs::copy_options::overwrite_existing,
			ec
		);

		if(ec) {
			ecsact::cli::report_error(
				"Failed to copy tracy header from runfiles. {} -> {}\n{}",
				full_hdr_path,
				(tracy_dir / rel_hdr_path).generic_string(),
				ec.message()
			);
			return false;
		}
	}
	return true;
}
