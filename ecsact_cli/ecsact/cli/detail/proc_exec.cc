#include "ecsact/cli/detail/proc_exec.hh"

#include <filesystem>
#include <span>
#include <boost/process.hpp>
#include "ecsact/cli/report.hh"

namespace bp = boost::process;
namespace fs = std::filesystem;

using ecsact::cli::subcommand_end_message;
using ecsact::cli::subcommand_id_t;
using ecsact::cli::subcommand_start_message;

auto ecsact::cli::detail::which(std::string_view prog)
	-> std::optional<fs::path> {
	auto result = bp::search_path(prog);

	if(result.empty()) {
		return std::nullopt;
	} else {
		return fs::path{result};
	}
}

auto ecsact::cli::detail::spawn_and_report( //
	std::filesystem::path    exe,
	std::vector<std::string> args,
	spawn_reporter&          reporter,
	std::filesystem::path    start_dir
) -> int {
	auto proc_stdout = bp::ipstream{};
	auto proc_stderr = bp::ipstream{};

	auto proc = bp::child{
		bp::exe(fs::absolute(exe).string()),
		bp::start_dir(start_dir.string()),
		bp::args(args),
		bp::std_out > proc_stdout,
		bp::std_err > proc_stderr,
	};

	auto subcommand_id = static_cast<subcommand_id_t>(proc.id());

	ecsact::cli::report(subcommand_start_message{
		.id = subcommand_id,
		.executable = exe.string(),
		.arguments = args,
	});

	auto line = std::string{};

	while(proc_stdout && std::getline(proc_stdout, line)) {
		auto msg = reporter.on_std_out(line).value_or(subcommand_stdout_message{
			.id = subcommand_id,
			.line = line,
		});
		ecsact::cli::report(msg);
	}

	while(proc_stderr && std::getline(proc_stderr, line)) {
		auto msg = reporter.on_std_err(line).value_or(subcommand_stderr_message{
			.id = subcommand_id,
			.line = line,
		});
		ecsact::cli::report(msg);
	}

	proc.wait();

	auto proc_exit_code = proc.exit_code();

	ecsact::cli::report(subcommand_end_message{
		.id = subcommand_id,
		.exit_code = proc_exit_code,
	});

	return proc_exit_code;
}

auto ecsact::cli::detail::spawn_and_report_output( //
	std::filesystem::path    exe,
	std::vector<std::string> args,
	std::filesystem::path    start_dir
) -> int {
	auto proc_stdout = bp::ipstream{};
	auto proc_stderr = bp::ipstream{};

	auto proc = bp::child{
		bp::exe(fs::absolute(exe).string()),
		bp::start_dir(start_dir.string()),
		bp::args(args),
		bp::std_out > proc_stdout,
		bp::std_err > proc_stderr,
	};

	auto subcommand_id = static_cast<subcommand_id_t>(proc.id());

	ecsact::cli::report(subcommand_start_message{
		.id = subcommand_id,
		.executable = exe.string(),
		.arguments = args,
	});

	ecsact::cli::report_stdout(subcommand_id, proc_stdout);
	ecsact::cli::report_stderr(subcommand_id, proc_stderr);

	proc.wait();

	auto proc_exit_code = proc.exit_code();

	ecsact::cli::report(subcommand_end_message{
		.id = subcommand_id,
		.exit_code = proc_exit_code,
	});

	return proc_exit_code;
}

auto ecsact::cli::detail::spawn_get_stdout( //
	std::filesystem::path    exe,
	std::vector<std::string> args,
	fs::path                 start_dir
) -> std::optional<std::string> {
	auto proc_stdout = bp::ipstream{};
	auto proc = bp::child{
		bp::exe(fs::absolute(exe)),
		bp::args(args),
		bp::start_dir(start_dir.string()),
		bp::std_out > proc_stdout,
	};

	auto proc_stdout_string = std::string{};

	while(proc_stdout) {
		auto line = std::string{};
		std::getline(proc_stdout, line);
		proc_stdout_string += line;
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
	auto proc_stdout = bp::ipstream{};
	auto proc = bp::child{
		bp::exe(fs::absolute(exe)),
		bp::args(args),
		bp::start_dir(start_dir.string()),
		bp::std_out > proc_stdout,
	};

	auto proc_stdout_bytes = std::vector<std::byte>{};
	auto proc_stdout_bytes_buf = std::array<std::byte, 1024>{};

	while(proc_stdout) {
		proc_stdout.read(
			reinterpret_cast<char*>(proc_stdout_bytes_buf.data()),
			proc_stdout_bytes_buf.size()
		);

		auto read_amount = proc_stdout.gcount();
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

	proc.wait();

	if(proc.exit_code() != 0) {
		return std::nullopt;
	}

	return proc_stdout_bytes;
}
