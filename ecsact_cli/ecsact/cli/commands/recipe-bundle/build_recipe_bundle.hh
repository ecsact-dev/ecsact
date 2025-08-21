#pragma once

#include <vector>
#include <cstddef>
#include <variant>
#include <stdexcept>
#include <filesystem>
#include "ecsact/cli/commands/build/build_recipe.hh"

namespace ecsact {

struct create_build_recipe_bundle_result;

class build_recipe_bundle {
	std::vector<std::byte> _bundle_bytes;
	build_recipe_bundle();

public:
	struct create_result;
	struct extract_result;
	static auto create(const build_recipe& recipe) -> create_result;
	static auto from_file(std::filesystem::path) -> create_result;

	build_recipe_bundle(build_recipe_bundle&&);
	~build_recipe_bundle();

	auto bytes() const -> std::span<const std::byte>;
	auto extract(std::filesystem::path extract_dir) const -> extract_result;
};

struct build_recipe_bundle::create_result
	: std::variant<build_recipe_bundle, std::logic_error> {
	using variant::variant;

	inline operator bool() {
		return std::holds_alternative<build_recipe_bundle>(*this);
	}

	inline auto operator*() & -> build_recipe_bundle& {
		return std::get<build_recipe_bundle>(*this);
	}

	inline auto operator->() & -> build_recipe_bundle* {
		return &std::get<build_recipe_bundle>(*this);
	}

	inline auto operator*() && -> build_recipe_bundle {
		return std::move(std::get<build_recipe_bundle>(*this));
	}

	inline auto error() -> std::logic_error {
		return std::get<std::logic_error>(*this);
	}
};

struct build_recipe_bundle::extract_result
	: std::variant<std::filesystem::path, std::logic_error> {
	using variant::variant;

	inline operator bool() {
		return std::holds_alternative<std::filesystem::path>(*this);
	}

	inline auto recipe_path() const -> std::filesystem::path {
		return std::get<std::filesystem::path>(*this);
	}

	inline auto error() -> std::logic_error {
		return std::get<std::logic_error>(*this);
	}
};

} // namespace ecsact
