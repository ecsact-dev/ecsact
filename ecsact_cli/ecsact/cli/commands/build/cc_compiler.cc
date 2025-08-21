#include "ecsact/cli/commands/build/cc_compiler.hh"

#include <filesystem>
#include <string>
#include <string_view>
#include <fstream>
#include <boost/process.hpp>
#include "nlohmann/json.hpp"
#include "ecsact/cli/commands/build/cc_compiler.hh"
#include "ecsact/cli/commands/build/cc_compiler_util.hh"
#include "ecsact/cli/report.hh"

using ecsact::cli::subcommand_end_message;
using ecsact::cli::subcommand_start_message;

namespace fs = std::filesystem;
namespace bp = boost::process;

static auto cc_from_string( //
	std::string_view str,
	fs::path         work_dir
) -> std::optional<ecsact::cli::cc_compiler> {
	auto compiler_path = fs::exists(str) //
		? fs::path{str}
		: fs::path{bp::search_path(str)};

	if(compiler_path.empty()) {
		return {};
	}

	auto compiler_type = ecsact::cli::get_compiler_type_by_path(compiler_path);

	if(!is_gcc_clang_like(compiler_type) &&
		 compiler_type != ecsact::cli::cc_compiler_type::clang_cl) {
		ecsact::cli::report_info("Compiler path: {}", compiler_path.string());
		ecsact::cli::report_warning(
			"Getting compiler info from path for {} is not supported",
			to_string(compiler_type)
		);
		return {};
	}

	auto compiler_version = ecsact::cli::compiler_version_string(compiler_path);

	if(compiler_version.empty()) {
		ecsact::cli::report_warning(
			"Failed to find compiler version with --version flag for {}",
			compiler_path.generic_string()
		);
		return {};
	}

	return ecsact::cli::cc_compiler{
		.compiler_type = compiler_type,
		.compiler_path = compiler_path,
		.compiler_version = compiler_version,
	// TODO(zaucy): Detect actual output extension
#ifdef _WIN32
		.preferred_output_extension = ".dll",
		.allowed_output_extensions = {".dll"},
#else
		.preferred_output_extension = ".so",
		.allowed_output_extensions = {".so"},
#endif
	};
}

static auto cc_from_env( //
	fs::path work_dir
) -> std::optional<ecsact::cli::cc_compiler> {
	auto compiler = std::optional<ecsact::cli::cc_compiler>{};
	auto cxx_env = std::getenv("CXX");

	if(cxx_env != nullptr) {
		compiler = cc_from_string(
			std::string_view{
				cxx_env,
				std::strlen(cxx_env),
			},
			work_dir
		);

		if(!compiler) {
			ecsact::cli::report_warning(
				"CXX environment variable was set, but was invalid"
			);
		}
	}

	if(!compiler) {
		auto cc_env = std::getenv("CC");

		if(cc_env != nullptr) {
			compiler = cc_from_string(
				std::string_view{
					cc_env,
					std::strlen(cc_env),
				},
				work_dir
			);

			if(!compiler) {
				ecsact::cli::report_warning(
					"CC environment variable was set, but was invalid"
				);
			}
		}
	}

	return compiler;
}

#ifdef _WIN32
static auto vsdevcmd_env_var(
	const fs::path&    vsdevcmd_path,
	const std::string& env_var_name
) -> std::vector<std::string> {
	auto proc_env = boost::this_process::environment();
	auto result = std::vector<std::string>{};
	auto std_is = bp::ipstream{};
	auto err_is = bp::ipstream{};
	auto extract_script_proc = bp::child{
		fs::absolute(vsdevcmd_path).string(),
		bp::args({env_var_name}),
		bp::std_out > std_is,
		bp::std_err > err_is,
		bp::std_in < bp::null,
	};

	auto subcommand_id =
		static_cast<ecsact::cli::subcommand_id_t>(extract_script_proc.id());
	ecsact::cli::report(subcommand_start_message{
		.id = subcommand_id,
		.executable = vsdevcmd_path.string(),
		.arguments = {env_var_name},
	});

	auto line = std::string{};
	while(std::getline(std_is, line, ';')) {
		boost::trim_right(line);
		if(!line.empty()) {
			result.emplace_back(line);
		}
	}

	while(std::getline(err_is, line)) {
		boost::trim(line);
		if(!line.empty()) {
			ecsact::cli::report(ecsact::cli::subcommand_stderr_message{
				.id = subcommand_id,
				.line = line
			});
		}
	}

	extract_script_proc.wait();

	ecsact::cli::report(subcommand_end_message{
		.id = subcommand_id,
		.exit_code = extract_script_proc.exit_code(),
	});

	return result;
}

static auto find_vswhere() -> std::optional<fs::path> {
	auto vswhere_path = std::string{};

	if(auto program_files = std::getenv("ProgramFiles(x86)"); program_files) {
		// https://github.com/Microsoft/vswhere/wiki/Installing
		vswhere_path = //
			(fs::path(program_files) / "Microsoft Visual Studio" / "Installer" /
			 "vswhere.exe")
				.string();

		if(!fs::exists(vswhere_path)) {
			vswhere_path = "";
		}
	}

	if(vswhere_path.empty()) {
		vswhere_path = bp::search_path("vswhere.exe").string();
	}

	if(vswhere_path.empty()) {
		ecsact::cli::report_error("Unable to find vswhere.exe");
		return {};
	}

	return fs::path{vswhere_path};
}
#endif // _WIN32

auto as_vec_path(auto&& vec) -> std::vector<fs::path> {
	auto result = std::vector<fs::path>{};
	result.reserve(vec.size());
	for(auto entry : vec) {
		result.emplace_back(entry);
	}

	return result;
}

#ifdef _WIN32
static auto cc_vswhere( //
	fs::path work_dir
) -> std::optional<ecsact::cli::cc_compiler> {
	auto vswhere = find_vswhere();
	if(!vswhere) {
		return {};
	}

	auto required_vs_components = std::vector<std::string>{
		"Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
		"Microsoft.VisualStudio.Component.Windows10SDK",
	};

	auto vswhere_proc_args = std::vector<std::string>{"-format", "json"};
	vswhere_proc_args.reserve(
		vswhere_proc_args.capacity() + (required_vs_components.size() * 2)
	);

	for(auto req_comp : required_vs_components) {
		vswhere_proc_args.emplace_back("-requires");
		vswhere_proc_args.emplace_back(req_comp);
	}

	auto vswhere_output_stream = bp::ipstream{};
	auto vswhere_proc = bp::child{
		bp::exe(vswhere->string()),
		bp::args(vswhere_proc_args),
		bp::std_out > vswhere_output_stream,
		bp::std_err > bp::null,
	};

	auto subcommand_id =
		static_cast<ecsact::cli::subcommand_id_t>(vswhere_proc.id());
	ecsact::cli::report(subcommand_start_message{
		.id = subcommand_id,
		.executable = vswhere->generic_string(),
		.arguments = vswhere_proc_args,
	});

	auto vswhere_output = nlohmann::json::parse(vswhere_output_stream);

	vswhere_proc.wait();
	auto vswhere_exit_code = vswhere_proc.exit_code();

	ecsact::cli::report(subcommand_end_message{
		.id = subcommand_id,
		.exit_code = vswhere_exit_code,
	});

	if(!vswhere_output.is_array()) {
		ecsact::cli::report_error(
			"Exppected JSON array in vswhere output. Instead got the following: {}",
			vswhere_output.dump(2, ' ')
		);
		return {};
	}

	auto vs_config_itr = vswhere_output.begin();
	if(vs_config_itr == vswhere_output.end()) {
		auto components_list_string = std::string{};

		for(auto& req_comp : required_vs_components) {
			components_list_string += std::format(" - {}\n", req_comp);
		}

		ecsact::cli::report_error(
			"Could not find Visual Studio installation with vswhere. Make sure "
			"you have the following components installed:\n{}",
			components_list_string
		);

		return {};
	}

	std::string vs_installation_path = vs_config_itr->at("installationPath");

	const std::string vsdevcmd_path =
		vs_installation_path + "\\Common7\\Tools\\vsdevcmd.bat";
	const auto vs_extract_env_path = work_dir / "vs_extract_env.bat";

	{
		auto env_extract_script_stream = std::ofstream{vs_extract_env_path};
		env_extract_script_stream //
			<< "@echo off\n"
			<< "setlocal EnableDelayedExpansion\n"
			<< "call \"" << vsdevcmd_path << "\" -arch=x64 > NUL\n"
			<< "echo !%*!\n";
		env_extract_script_stream.flush();
		env_extract_script_stream.close();
	}

	// Quick labmda for convenience
	auto vsdevcmd_env_varl = [&](const std::string& env_var) {
		return vsdevcmd_env_var(vs_extract_env_path, env_var);
	};

	auto standard_include_paths = vsdevcmd_env_varl("INCLUDE");
	if(standard_include_paths.empty()) {
		ecsact::cli::report_error("Extracted INCLUDE paths is empty");
		return {};
	}

	auto standard_lib_paths = vsdevcmd_env_varl("LIB");
	if(standard_lib_paths.empty()) {
		ecsact::cli::report_error("Extracted LIB paths is empty");
		return {};
	}

	// https://github.com/microsoft/vswhere/wiki/Find-VC
	auto version_text_path = std::format(
		"{}\\VC\\Auxiliary\\Build\\Microsoft.VCToolsVersion.default.txt",
		vs_installation_path
	);
	auto tools_version = std::string{};
	{
		auto version_text_stream = std::ifstream{version_text_path};
		std::getline(version_text_stream, tools_version);
	}

	if(tools_version.empty()) {
		ecsact::cli::report_error("Unable to read {}", version_text_path);
		return {};
	}

	auto cl_path = std::format(
		"{}\\VC\\Tools\\MSVC\\{}\\bin\\HostX64\\x64\\cl.exe",
		vs_installation_path,
		tools_version
	);

	return ecsact::cli::cc_compiler{
		.compiler_type = ecsact::cli::cc_compiler_type::msvc_cl,
		.compiler_path = cl_path,
		.compiler_version = tools_version,
		.std_inc_paths = as_vec_path(standard_include_paths),
		.std_lib_paths = as_vec_path(standard_lib_paths),
		.preferred_output_extension = ".dll",
		.allowed_output_extensions = {".dll"},
	};
}
#endif

static auto cc_default( //
	fs::path work_dir
) -> std::optional<ecsact::cli::cc_compiler> {
#ifdef _WIN32
	return cc_vswhere(work_dir);
#else
	return cc_from_string("clang", work_dir);
#endif
}

static auto cc_from_env_path( //
	fs::path work_dir
) -> std::optional<ecsact::cli::cc_compiler> {
	return {};
}

auto ecsact::cli::detect_cc_compiler( //
	fs::path work_dir
) -> std::optional<cc_compiler> {
	auto compiler = cc_from_env(work_dir);

	if(!compiler) {
		compiler = cc_default(work_dir);
	}

	if(!compiler) {
		compiler = cc_from_env_path(work_dir);
	}

	return compiler;
}

auto ecsact::cli::find_cc_compiler(
	std::filesystem::path work_dir,
	std::string           compiler_name_or_path
) -> std::optional<cc_compiler> {
	return cc_from_string(compiler_name_or_path, work_dir);
}
