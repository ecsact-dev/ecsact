#include "ecsact/cli/commands/build/get_modules.hh"

#include "ecsact/runtime/async.h"
#include "ecsact/runtime/core.h"
#include "ecsact/runtime/dynamic.h"
#include "ecsact/runtime/meta.h"
#include "ecsact/runtime/serialize.h"
#include "ecsact/si/wasm.h"

auto ecsact::cli::detail::get_ecsact_modules( //
	std::vector<std::string> methods
) -> get_ecsact_modules_result {
	auto result = get_ecsact_modules_result{};

#define CHECK_MODULE(fn, mod)                     \
	if(method == #fn) {                             \
		result.module_methods[mod].emplace_back(#fn); \
		continue;                                     \
	}                                               \
	static_assert(true, "")

	for(auto method : methods) {
		FOR_EACH_ECSACT_ASYNC_API_FN(CHECK_MODULE, "async");
		FOR_EACH_ECSACT_CORE_API_FN(CHECK_MODULE, "core");
		FOR_EACH_ECSACT_DYNAMIC_API_FN(CHECK_MODULE, "dynamic");
		FOR_EACH_ECSACT_META_API_FN(CHECK_MODULE, "meta");
		FOR_EACH_ECSACT_SERIALIZE_API_FN(CHECK_MODULE, "serialize");
		FOR_EACH_ECSACT_SI_WASM_API_FN(CHECK_MODULE, "si_wasm");

		result.unknown_module_methods.emplace_back(method);
	}

#undef CHECK_MODULE

	return result;
}
