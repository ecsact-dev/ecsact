#include "ecsact/cli/commands/build/cc_defines_gen.hh"

#include <ranges>
#include <cassert>
#include <format>
#include <boost/algorithm/string/case_conv.hpp>
#include "ecsact/cli/commands/build/get_modules.hh"

auto ecsact::cli::cc_defines_gen( //
	std::vector<std::string> imports,
	std::vector<std::string> exports
) -> std::vector<std::string> {
	auto import_modules = ecsact::cli::detail::get_ecsact_modules(imports);
	auto export_modules = ecsact::cli::detail::get_ecsact_modules(exports);

	// these should have been checked before
	assert(import_modules.unknown_module_methods.empty());
	assert(export_modules.unknown_module_methods.empty());

	auto defines = std::vector<std::string>{};

	for(auto&& [mod, _] : export_modules.module_methods) {
		defines.push_back(std::format( //
			"ECSACT_{}_API_EXPORT",
			boost::to_upper_copy(mod)
		));
	}

	for(auto&& [mod, _] : import_modules.module_methods) {
		defines.push_back(std::format( //
			"ECSACT_{}_API_LOAD_AT_RUNTIME",
			boost::to_upper_copy(mod)
		));
	}
	return defines;
}
