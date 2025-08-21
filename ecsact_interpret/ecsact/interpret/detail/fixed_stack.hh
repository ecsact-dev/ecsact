#pragma once

#include <array>
#include <cstddef>
#include <cassert>

namespace ecsact::detail {

template<typename T, std::size_t MaxSize>
class fixed_stack {
	std::size_t            _size = 0;
	std::array<T, MaxSize> _data;

public:
	using value_type = T;
	using size_type = std::size_t;
	using reference = T&;
	using const_reference = const T&;

	[[nodiscard]] constexpr T* data() noexcept {
		return _data.data();
	}

	[[nodiscard]] constexpr T* data() const noexcept {
		return _data.data();
	}

	[[nodiscard]] constexpr reference top() {
		assert(_size > 0);
		return _data[_size - 1];
	}

	[[nodiscard]] constexpr const_reference top() const {
		assert(_size > 0);
		return _data[_size - 1];
	}

	[[nodiscard]] constexpr size_type size() const noexcept {
		return _size;
	}

	[[nodiscard]] constexpr bool empty() const noexcept {
		return _size == 0;
	}

	[[nodiscard]] constexpr size_type max_size() const noexcept {
		return MaxSize;
	}

	constexpr void push(const value_type& value) {
		assert(_size < MaxSize);
		_data[_size++] = value;
	}

	constexpr void push(value_type&& value) {
		assert(_size < MaxSize);
		_data[_size++] = value;
	}

	template<typename... Args>
	constexpr decltype(auto) emplace(Args&&... args) {
		assert(_size < MaxSize);
		return _data[_size++] = value_type{std::forward<Args>(args)...};
	}

	constexpr void pop() noexcept {
		assert(_size > 0);
		--_size;
	}

	constexpr void swap(fixed_stack& other) noexcept {
		using std::swap;
		swap(_size, other._size);
		swap(_data, other._data);
	}

	constexpr void clear() {
		_size = 0;
	}
};

} // namespace ecsact::detail
