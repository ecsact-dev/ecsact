#pragma once

#include <variant>
#include <vector>
#include <cstddef>
#include <stdexcept>
#include <filesystem>

namespace ecsact::cli::detail {

using download_file_buffer_t = std::vector<std::byte>;

struct download_file_result
	: std::variant<std::vector<std::byte>, std::logic_error> {
	using variant::variant;

	using result_type = std::vector<std::byte>;
	using error_type = std::logic_error;

	inline operator bool() {
		return std::holds_alternative<result_type>(*this);
	}

	inline auto operator*() & -> result_type& {
		return std::get<result_type>(*this);
	}

	inline auto operator->() & -> result_type* {
		return &std::get<result_type>(*this);
	}

	inline auto operator*() && -> result_type {
		return std::move(std::get<result_type>(*this));
	}

	inline auto error() -> error_type {
		return std::get<error_type>(*this);
	}
};

auto download_file( //
	std::string_view url
) -> download_file_result;

} // namespace ecsact::cli::detail
