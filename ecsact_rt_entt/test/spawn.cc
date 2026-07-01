#include "spawn.hh"

#include <boost/process.hpp>
#include <boost/asio.hpp>

namespace bp = boost::process;

int ecsact::entt::test::detail::spawn(
	std::string              executable,
	std::vector<std::string> args
) {
	auto ioc = boost::asio::io_context{};
	auto proc = bp::process{
		ioc,
		executable,
		args,
		bp::process_stdio{nullptr, stdout, stderr},
	};

	ioc.run();

	proc.wait();
	return proc.exit_code();
}
