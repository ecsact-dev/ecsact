#pragma once

#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <array>
#include <cstddef>

namespace ecsact::cli::detail {
struct integrity_unknown {
	static auto from_bytes( //
		const std::span<const std::byte> bytes
	) -> integrity_unknown;

	constexpr auto data() -> std::byte* {
		return nullptr;
	}

	constexpr auto data() const -> const std::byte* {
		return nullptr;
	}

	constexpr auto size() const -> std::size_t {
		return 0;
	}

	constexpr auto to_string() const -> std::string {
		return "unknown";
	}

	constexpr auto operator==(const integrity_unknown&) const -> bool {
		return false;
	}
};

struct integrity_sha256 {
	static constexpr auto string_prefix() -> std::string_view {
		return "sha256-";
	}

	static auto from_bytes( //
		const std::span<const std::byte> bytes
	) -> integrity_sha256;

	std::array<std::byte, 32> _data;

	constexpr auto operator==(const integrity_sha256& other) const -> bool {
		return _data == other._data;
	}

	constexpr auto data() -> std::byte* {
		return _data.data();
	}

	constexpr auto data() const -> const std::byte* {
		return _data.data();
	}

	constexpr auto size() const -> std::size_t {
		return _data.size();
	}

	auto to_string() const -> std::string;
};

struct integrity : std::variant<integrity_unknown, integrity_sha256> {
	using variant::variant;

	enum kind {
		unknown,
		sha256,
	};

	static auto from_bytes( //
		enum kind                        kind,
		const std::span<const std::byte> bytes
	) -> integrity;

	static auto from_string(std::string_view str) -> integrity;

	constexpr operator bool() const {
		return index() > 0;
	}

	constexpr auto kind() const -> kind {
		return static_cast<enum kind>(index());
	}

	constexpr auto data() const -> const std::byte* {
		return std::visit([](const auto& v) { return v.data(); }, *this);
	}

	constexpr auto data() -> std::byte* {
		return std::visit([](auto& v) { return v.data(); }, *this);
	}

	constexpr auto size() const -> std::size_t {
		return std::visit([](const auto& v) { return v.size(); }, *this);
	}

	constexpr auto operator==( //
		const integrity& other
	) const -> bool {
		if(kind() == other.kind()) {
			return std::visit(
				[&other](const auto& self) {
					return self == std::get<std::remove_cvref_t<decltype(self)>>(other);
				},
				*this
			);
		}

		return false;
	}

	inline auto to_string() const -> std::string {
		return std::visit([](auto& v) { return v.to_string(); }, *this);
	}
};
} // namespace ecsact::cli::detail
