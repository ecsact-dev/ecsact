#include "spawn.hh"

#include <boost/process.hpp>

namespace bp = boost::process;

int ecsact::entt::test::detail::spawn(
	std::string              executable,
	std::vector<std::string> args
) {
	auto proc = bp::child(
		bp::exe(executable),
		bp::args(args),
		bp::std_out > stdout,
		bp::std_err > stderr
	);

	proc.wait();
	return proc.exit_code();
}
