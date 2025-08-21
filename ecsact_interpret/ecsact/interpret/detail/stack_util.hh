#pragma once

#include <optional>
#include <functional>
#include <type_traits>

namespace ecsact::detail {

[[nodiscard]] auto try_top(auto& stack) {
	static_assert(std::is_reference_v<decltype(stack)>);
	using value_type = std::remove_reference_t<decltype(stack.top())>;

	std::optional<std::reference_wrapper<value_type>> result;
	if(!stack.empty()) {
		result = std::ref(stack.top());
	}
	return result;
}

} // namespace ecsact::detail
