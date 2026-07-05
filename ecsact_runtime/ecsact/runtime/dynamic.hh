#ifndef ECSACT_RUNTIME_DYNAMIC_HH
#define ECSACT_RUNTIME_DYNAMIC_HH

#include "ecsact/runtime/dynamic.hh"

namespace ecsact::dynamic {

template<typename S>
auto set_default_system_execution_impl() -> bool {
	return ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(S::id),
		S::get_c_impl()
	);
}

template<typename S>
auto clear_system_execution_impl() -> bool {
	return ecsact_set_system_execution_impl(
		ecsact_id_cast<ecsact_system_like_id>(S::id),
		nullptr
	);
}
} // namespace ecsact::dynamic

#endif // ECSACT_RUNTIME_DYNAMIC_HH
