#include "ecsact/cli/commands/build/cc_compiler_util.hh"

#include <string>
#include <boost/process.hpp>

namespace fs = std::filesystem;
namespace bp = boost::process;

auto ecsact::cli::compiler_version_string( //
	fs::path compiler_path
) -> std::string {
	auto version_stdout = bp::ipstream{};
	auto version_child_proc = bp::child{
		bp::exe(compiler_path.string()),
		bp::args("--version"),
		bp::std_out > version_stdout,
	};

	auto compiler_version_str = std::string{};
	std::getline(version_stdout, compiler_version_str);
	version_child_proc.wait();

	return compiler_version_str;
}
