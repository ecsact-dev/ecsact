#include <windows.h> // windows header always comes first
#include "bazel/runfiles/runfiles.hh"

#include <string>

using bazel_sundry::Runfiles;

std::unique_ptr<Runfiles> bazel_sundry::CreateDefaultRunfiles() {
	char result[MAX_PATH];
	size_t length = GetModuleFileName(NULL, result, MAX_PATH);
	if(length == 0) {
		return nullptr;
	}

	Runfiles* runfiles = Runfiles::Create(std::string(result));
	if(!runfiles) {
		return nullptr;
	}

	return std::unique_ptr<Runfiles>(runfiles);
}
