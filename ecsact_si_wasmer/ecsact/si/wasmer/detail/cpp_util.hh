#pragma once

#include <utility>

#ifndef defer
struct deferred_invoker {};

template<typename F>
struct deferred {
	F _defer_fn;

	~deferred() {
		_defer_fn();
	}
};

template<typename F>
deferred<F> operator*(deferred_invoker, F&& f) {
	return {std::forward<F>(f)};
}

/**
 * USAGE: defer { statements; }
 */
#	define defer auto _deferred##__LINE__ = deferred_invoker{}* [&]()
#endif // defer

template<typename... Ts>
struct overloaded : Ts... {
	using Ts::operator()...;
};

template<typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>;
