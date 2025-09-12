#include "bazel/runfiles/runfiles.hh"

#include <string>
#include <filesystem>

using bazel_sundry::Runfiles;
namespace fs = std::filesystem;

std::unique_ptr<Runfiles> bazel_sundry::CreateDefaultRunfiles() {
	std::error_code ec;
	auto result = fs::canonical("/proc/self/exe", ec);
	if(ec) {
		return nullptr;
	}

	Runfiles* runfiles = Runfiles::Create(result.string());
	if(!runfiles) {
		return nullptr;
	}

	return std::unique_ptr<Runfiles>(runfiles);
}
