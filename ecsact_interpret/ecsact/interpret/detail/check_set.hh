#pragma once

#include <vector>

namespace ecsact::detail {

class check_set {
	int               _checked_count = 0;
	std::vector<bool> _data;

public:
	using size_type = decltype(_data.size());

	inline size_type size() const noexcept {
		return _data.size();
	}

	inline void reserve(size_type new_capacity) {
		_data.reserve(new_capacity);
	}

	inline size_type capacity() const noexcept {
		return _data.capacity();
	}

	inline void resize(size_type new_size) {
		if(new_size < _data.size()) {
			_checked_count = 0;
			for(size_type i = 0; new_size > i; ++i) {
				if(_data[i]) {
					++_checked_count;
				}
			}
		}
		_data.resize(new_size, false);
	}

	inline bool get(size_type index) const noexcept {
		return _data[index];
	}

	inline void check(size_type index) {
		if(!_data[index]) {
			++_checked_count;
			_data[index] = true;
		}
	}

	inline void uncheck(size_type index) {
		if(_data[index]) {
			--_checked_count;
			_data[index] = false;
		}
	}

	inline void toggle(size_type index) {
		if(_data[index]) {
			--_checked_count;
			_data[index] = false;
		} else {
			++_checked_count;
			_data[index] = true;
		}
	}

	inline void set(size_type index, bool checked) {
		if(checked && !get(index)) {
			check(index);
		} else if(!checked && get(index)) {
			uncheck(index);
		}
	}

	inline bool complete() const noexcept {
		return _checked_count == _data.size();
	}
};

} // namespace ecsact::detail
