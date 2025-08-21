#pragma once

#include <filesystem>
#include <vector>
#include <string_view>
#include <string>
#include <optional>
#include "ecsact/cli/report_message.hh"

namespace ecsact::cli::detail {

/**
 * Lookup program on PATH
 */
auto which( //
	std::string_view prog
) -> std::optional<std::filesystem::path>;

struct spawn_reporter {
	virtual auto on_std_out( //
		std::string_view line
	) -> std::optional<ecsact::cli::message_variant_t> {
		return {};
	}

	virtual auto on_std_err( //
		std::string_view line
	) -> std::optional<ecsact::cli::message_variant_t> {
		return {};
	}
};

auto spawn_and_report(
	std::filesystem::path    exe,
	std::vector<std::string> args,
	spawn_reporter&          reporter,
	std::filesystem::path    start_dir = std::filesystem::current_path()
) -> int;

/**
 * Spawn a process and report the stdout/stderr
 * @returns exit code
 */
auto spawn_and_report_output( //
	std::filesystem::path    exe,
	std::vector<std::string> args,
	std::filesystem::path    start_dir = std::filesystem::current_path()
) -> int;

/**
 * Spawn a process and return the stdout
 * @returns `std::nullopt` if spawn failed
 */
auto spawn_get_stdout( //
	std::filesystem::path    exe,
	std::vector<std::string> args,
	std::filesystem::path    start_dir = std::filesystem::current_path()
) -> std::optional<std::string>;

auto spawn_get_stdout_bytes( //
	std::filesystem::path    exe,
	std::vector<std::string> args,
	std::filesystem::path    start_dir = std::filesystem::current_path()
) -> std::optional<std::vector<std::byte>>;
} // namespace ecsact::cli::detail
