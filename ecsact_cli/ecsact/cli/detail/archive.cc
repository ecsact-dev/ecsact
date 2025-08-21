#include "ecsact/cli/detail/archive.hh"

#include <memory>
#include <stdexcept>
#include <format>
#include <filesystem>
#ifdef __cpp_lib_stacktrace
#	include <stacktrace>
#endif
#include <archive.h>
#include <archive_entry.h>

using namespace std::string_literals;
using namespace std::string_view_literals;
namespace fs = std::filesystem;

static auto archive_error_as_logic_error(archive* a) -> std::logic_error {
	auto msg = std::string{archive_error_string(a)};

#ifdef __cpp_lib_stacktrace
	msg += "\n";
	msg += std::to_string(std::stacktrace::current(1));
#endif

	return std::logic_error{msg};
}

auto ecsact::cli::detail::read_archive( //
	const std::span<const std::byte> archive_bytes,
	read_archive_callback_t          read_callback
) -> void {
	auto result = int{};
	auto a = std::unique_ptr<archive, decltype(&archive_read_free)>{
		archive_read_new(),
		archive_read_free,
	};

	result = archive_read_support_filter_all(a.get());
	if(result != ARCHIVE_OK) {
		throw archive_error_as_logic_error(a.get());
	}
	result = archive_read_support_format_all(a.get());
	if(result != ARCHIVE_OK) {
		throw archive_error_as_logic_error(a.get());
	}

	result = archive_read_open_memory(
		a.get(),
		archive_bytes.data(),
		archive_bytes.size()
	);
	if(result != ARCHIVE_OK) {
		throw archive_error_as_logic_error(a.get());
	}

	auto entry = archive_entry_new2(a.get());
	auto data = std::vector<std::byte>{};

	for(;;) {
		result = archive_read_next_header2(a.get(), entry);
		if(result == ARCHIVE_EOF) {
			break;
		}

		if(result < ARCHIVE_OK) {
			throw archive_error_as_logic_error(a.get());
		}

		auto path = std::string_view{archive_entry_pathname(entry)};
		auto size = archive_entry_size(entry);

		if(size == 0) {
			throw std::logic_error{std::format("No size for {}", path)};
		}

		data.resize(size);
		auto read_size = archive_read_data(a.get(), data.data(), data.size());

		if(read_size != size) {
			throw std::logic_error{std::format("Failed to read {}", path)};
		}

		if(path.starts_with("/")) {
			path = path.substr(1);
		}

		read_callback(path, std::span{data.data(), data.size()});
	}

	archive_entry_free(entry);
}
