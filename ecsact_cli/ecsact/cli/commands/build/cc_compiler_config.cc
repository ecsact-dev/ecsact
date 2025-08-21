#include "ecsact/cli/commands/build/cc_compiler_config.hh"

#include <fstream>
#include <filesystem>
#include "ecsact/cli/report.hh"
#include "ecsact/cli/commands/build/cc_compiler_util.hh"

namespace fs = std::filesystem;

auto ecsact::cli::get_compiler_type_by_path( //
	std::filesystem::path compiler_path
) -> cc_compiler_type {
	auto compiler_filename = compiler_path.filename().replace_extension("");

	if(compiler_filename == "cl") {
		return ecsact::cli::cc_compiler_type::msvc_cl;
	}
	if(compiler_filename == "clang-cl") {
		return ecsact::cli::cc_compiler_type::clang_cl;
	}

	if(compiler_filename == "clang") {
		return ecsact::cli::cc_compiler_type::clang;
	}

	if(compiler_filename == "clang++") {
		return ecsact::cli::cc_compiler_type::clang;
	}

	if(compiler_filename == "gcc") {
		return ecsact::cli::cc_compiler_type::gcc;
	}
	if(compiler_filename == "emcc") {
		return ecsact::cli::cc_compiler_type::emcc;
	}

	return ecsact::cli::cc_compiler_type::unknown;
}

auto ecsact::cli::to_string(cc_compiler_type type) -> std::string_view {
	switch(type) {
		case ecsact::cli::cc_compiler_type::msvc_cl:
			return "MSVC Compiler";
		case ecsact::cli::cc_compiler_type::clang_cl:
			return "clang-cl";
		case ecsact::cli::cc_compiler_type::clang:
			return "clang";
		case ecsact::cli::cc_compiler_type::gcc:
			return "gcc";
		case ecsact::cli::cc_compiler_type::emcc:
			return "emscripten";
		default:
			return "unknown";
	}
}

auto ecsact::cli::to_json( //
	nlohmann::json&    j,
	const cc_compiler& compiler
) -> void {
	j = nlohmann::json{
		{"compiler_path", compiler.compiler_path.string()},
		{"compiler_version", compiler.compiler_version},
		{"install_path", compiler.install_path.string()},
		{"preferred_output_extension", compiler.preferred_output_extension},
	};
}

template<typename T, typename Key>
static auto json_get_opt( //
	const nlohmann::json& j,
	Key                   key
) -> std::optional<T> {
	if(!j.contains(key)) {
		ecsact::cli::report_error(
			"Missing required field in compiler config: '{}'",
			key
		);
		return std::nullopt;
	}

	using value_type = std::remove_cvref_t<T>;

	auto value = j.at(key);

	if constexpr(std::is_same_v<value_type, std::string>) {
		if(!value.is_string()) {
			ecsact::cli::report_error(
				"Expected field '{}' to be of type string",
				key
			);
			return std::nullopt;
		}
		return value.template get<std::string>();
	} else if constexpr(std::is_same_v<value_type, fs::path>) {
		if(!value.is_string()) {
			ecsact::cli::report_error(
				"Expected field '{}' to be of a path (string)",
				key
			);
			return std::nullopt;
		}
		return fs::path{value.template get<std::string>()};
	} else if constexpr(std::is_same_v<value_type, std::vector<std::string>>) {
		if(!value.is_array()) {
			ecsact::cli::report_error("Expected field '{}' to array of strings", key);
			return std::nullopt;
		}

		return value.template get<std::vector<std::string>>();
	} else if constexpr(std::is_same_v<value_type, std::vector<fs::path>>) {
		if(!value.is_array()) {
			ecsact::cli::report_error(
				"Expected field '{}' to array of paths (strings)",
				key
			);
			return std::nullopt;
		}

		auto path_strings = value.template get<std::vector<std::string>>();

		auto paths = std::vector<fs::path>{};
		paths.reserve(path_strings.size());

		for(auto path_string : path_strings) {
			paths.emplace_back(path_string);
		}

		return paths;
	}

	return std::nullopt;
}

static auto validate_compiler_config_json( //
	const nlohmann::json& j
) -> std::optional<ecsact::cli::cc_compiler> {
	auto compiler = ecsact::cli::cc_compiler{};

	if(auto v = json_get_opt<fs::path>(j, "compiler_path"); !v) {
		return {};
	} else {
		compiler.compiler_path = *v;

		if(compiler.compiler_path.empty()) {
			ecsact::cli::report_error(
				"Configured compiler_path is empty. Must be a path to your compiler"
			);
			return {};
		}

		if(!fs::exists(compiler.compiler_path)) {
			ecsact::cli::report_error(
				"Configured compiler_path {} does not exist",
				compiler.compiler_path.string()
			);
			return {};
		}
	}

	if(auto v = json_get_opt<std::string>(j, "compiler_type"); !v) {
		return {};
	} else {
		if(v->empty() || *v == "auto") {
			compiler.compiler_type =
				ecsact::cli::get_compiler_type_by_path(compiler.compiler_path);
		} else {
			ecsact::cli::report_error("Invalid compiler type: {}", *v);
			return {};
		}
	}

	if(auto v = json_get_opt<std::string>(j, "compiler_version"); !v) {
		return {};
	} else {
		compiler.compiler_version = *v;
	}

	if(auto v = json_get_opt<fs::path>(j, "install_path"); !v) {
		return {};
	} else {
		compiler.install_path = *v;
	}

	if(auto v = json_get_opt<std::vector<fs::path>>(j, "std_inc_paths"); !v) {
		return {};
	} else {
		compiler.std_inc_paths = *v;
	}

	if(auto v = json_get_opt<std::vector<fs::path>>(j, "std_lib_paths"); !v) {
		return {};
	} else {
		compiler.std_lib_paths = *v;
	}

	if(auto v = json_get_opt<std::string>(j, "preferred_output_extension"); !v) {
		return {};
	} else {
		compiler.preferred_output_extension = *v;
	}

	if(auto v =
			 json_get_opt<std::vector<std::string>>(j, "allowed_output_extensions");
		 !v) {
		return {};
	} else {
		compiler.allowed_output_extensions = *v;
	}

	return compiler;
}

auto ecsact::cli::load_compiler_config( //
	std::filesystem::path config_path
) -> std::optional<cc_compiler> {
	auto config_stream = std::ifstream{config_path};

	if(!config_stream) {
		ecsact::cli::report_error("Failed to read {}", config_path.string());
		return std::nullopt;
	}

	auto compiler_config = nlohmann::json{};
	config_stream >> compiler_config;

	auto compiler = validate_compiler_config_json(compiler_config);

	if(!compiler) {
		return {};
	}

	// @HACK: sometimes the compiler type can be derived from the version flag
	if(compiler->compiler_type == cc_compiler_type::unknown) {
		auto version_string =
			ecsact::cli::compiler_version_string(compiler->compiler_path);

		if(version_string.find("clang") != std::string::npos) {
			compiler->compiler_type = cc_compiler_type::clang;
		} else if(version_string.find("gcc") != std::string::npos) {
			compiler->compiler_type = cc_compiler_type::gcc;
		}

		if(compiler->compiler_version.empty()) {
			compiler->compiler_version = version_string;
		}
	}

	return compiler;
}
