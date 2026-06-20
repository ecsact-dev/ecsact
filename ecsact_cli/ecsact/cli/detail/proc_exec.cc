#include "ecsact/cli/detail/proc_exec.hh"

#include <filesystem>
#include <span>
#include <boost/process.hpp>
#include <boost/asio.hpp>
#include "ecsact/cli/report.hh"

namespace bp = boost::process;
namespace fs = std::filesystem;

using ecsact::cli::subcommand_end_message;
using ecsact::cli::subcommand_id_t;
using ecsact::cli::subcommand_start_message;

auto ecsact::cli::detail::which(std::string_view prog)
	-> std::optional<fs::path> {
	auto result = bp::environment::find_executable(prog);

	if(result.empty()) {
		return std::nullopt;
	} else {
		return fs::path{result.string()};
	}
}

namespace {
template<typename ReporterFn>
auto read_pipe_lines(boost::asio::readable_pipe& pipe, ReporterFn reporter_fn) -> void {
	boost::system::error_code ec;
	boost::asio::streambuf buffer;
	while(!ec) {
		auto read_bytes = boost::asio::read_until(pipe, buffer, '\n', ec);
		if (read_bytes > 0 || buffer.size() > 0) {
			std::istream is(&buffer);
			auto line = std::string{};
			while(std::getline(is, line)) {
				reporter_fn(line);
			}
		}
	}
}
} // namespace

auto ecsact::cli::detail::spawn_and_report( //
	std::filesystem::path    exe,
	std::vector<std::string> args,
	spawn_reporter&          reporter,
	std::filesystem::path    start_dir
) -> int {
	auto ioc = boost::asio::io_context{};
	auto proc_stdout = boost::asio::readable_pipe{ioc};
	auto proc_stderr = boost::asio::readable_pipe{ioc};

	auto proc = bp::process{
		ioc,
		fs::absolute(exe).string(),
		args,
		bp::process_start_dir{start_dir.string()},
		bp::process_stdio{nullptr, proc_stdout, proc_stderr},
	};

	auto subcommand_id = static_cast<subcommand_id_t>(proc.id());

	ecsact::cli::report(
		subcommand_start_message{
			.id = subcommand_id,
			.executable = exe.string(),
			.arguments = args,
		}
	);

	read_pipe_lines(proc_stdout, [&](const std::string& line) {
		auto msg = reporter.on_std_out(line).value_or(
			subcommand_stdout_message{
				.id = subcommand_id,
				.line = line,
			}
		);
		ecsact::cli::report(msg);
	});

	read_pipe_lines(proc_stderr, [&](const std::string& line) {
		auto msg = reporter.on_std_err(line).value_or(
			subcommand_stderr_message{
				.id = subcommand_id,
				.line = line,
			}
		);
		ecsact::cli::report(msg);
	});

	proc.wait();

	auto proc_exit_code = proc.exit_code();

	ecsact::cli::report(
		subcommand_end_message{
			.id = subcommand_id,
			.exit_code = proc_exit_code,
		}
	);

	return proc_exit_code;
}

auto ecsact::cli::detail::spawn_and_report_output( //
	std::filesystem::path    exe,
	std::vector<std::string> args,
	std::filesystem::path    start_dir
) -> int {
	auto ioc = boost::asio::io_context{};
	auto proc_stdout = boost::asio::readable_pipe{ioc};
	auto proc_stderr = boost::asio::readable_pipe{ioc};

	auto proc = bp::process{
		ioc,
		fs::absolute(exe).string(),
		args,
		bp::process_start_dir{start_dir.string()},
		bp::process_stdio{nullptr, proc_stdout, proc_stderr},
	};

	auto subcommand_id = static_cast<subcommand_id_t>(proc.id());

	ecsact::cli::report(
		subcommand_start_message{
			.id = subcommand_id,
			.executable = exe.string(),
			.arguments = args,
		}
	);

	read_pipe_lines(proc_stdout, [&](const std::string& line) {
		ecsact::cli::report(
			subcommand_stdout_message{
				.id = subcommand_id,
				.line = line,
			}
		);
	});
	read_pipe_lines(proc_stderr, [&](const std::string& line) {
		ecsact::cli::report(
			subcommand_stderr_message{
				.id = subcommand_id,
				.line = line,
			}
		);
	});

	proc.wait();

	auto proc_exit_code = proc.exit_code();

	ecsact::cli::report(
		subcommand_end_message{
			.id = subcommand_id,
			.exit_code = proc_exit_code,
		}
	);

	return proc_exit_code;
}

auto ecsact::cli::detail::spawn_get_stdout( //
	std::filesystem::path    exe,
	std::vector<std::string> args,
	fs::path                 start_dir
) -> std::optional<std::string> {
	auto ioc = boost::asio::io_context{};
	auto proc_stdout = boost::asio::readable_pipe{ioc};
	auto proc = bp::process{
		ioc,
		fs::absolute(exe).string(),
		args,
		bp::process_start_dir{start_dir.string()},
		bp::process_stdio{nullptr, proc_stdout, nullptr},
	};

	auto proc_stdout_string = std::string{};
	boost::system::error_code ec;
	char buf[1024];
	while(!ec) {
		auto read_amount = proc_stdout.read_some(boost::asio::buffer(buf), ec);
		if(read_amount > 0) {
			proc_stdout_string.append(buf, read_amount);
		}
	}

	proc.wait();

	if(proc.exit_code() != 0) {
		return std::nullopt;
	}

	return proc_stdout_string;
}

auto ecsact::cli::detail::spawn_get_stdout_bytes( //
	std::filesystem::path    exe,
	std::vector<std::string> args,
	fs::path                 start_dir
) -> std::optional<std::vector<std::byte>> {
	auto ioc = boost::asio::io_context{};
	auto proc_stdout = boost::asio::readable_pipe{ioc};
	auto proc = bp::process{
		ioc,
		fs::absolute(exe).string(),
		args,
		bp::process_start_dir{start_dir.string()},
		bp::process_stdio{nullptr, proc_stdout, nullptr},
	};

	auto proc_stdout_bytes = std::vector<std::byte>{};
	auto proc_stdout_bytes_buf = std::array<std::byte, 1024>{};

	boost::system::error_code ec;
	while(!ec) {
		auto read_amount = proc_stdout.read_some(boost::asio::buffer(proc_stdout_bytes_buf), ec);
		if(read_amount > 0) {
			auto read_bytes = std::span{
				proc_stdout_bytes_buf.data(),
				static_cast<size_t>(read_amount),
			};

			proc_stdout_bytes.insert(
				proc_stdout_bytes.end(),
				read_bytes.data(),
				read_bytes.data() + read_bytes.size()
			);
		}
	}

	proc.wait();

	if(proc.exit_code() != 0) {
		return std::nullopt;
	}

	return proc_stdout_bytes;
}
