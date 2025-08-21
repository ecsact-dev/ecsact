#pragma once

#include <functional>
#include <string_view>
#include <span>
#include <cstddef>

namespace ecsact::cli::detail {

using read_archive_callback_t =
	std::function<void(std::string_view path, std::span<std::byte> data)>;

auto read_archive( //
	const std::span<const std::byte> archive_bytes,
	read_archive_callback_t          read_callback
) -> void;

} // namespace ecsact::cli::detail
