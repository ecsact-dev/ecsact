#pragma once

#include <type_traits>

namespace ecsact::detail {

void stream_get_until(auto& stream, auto& output, auto&& delimiters) {
	while(stream) {
		std::remove_reference_t<decltype(output.back())> c;
		stream.get(c);
		if(stream.eof()) {
			break;
		}

		output.push_back(c);

		for(auto& delim : delimiters) {
			if(c == delim) {
				return;
			}
		}
	}
}

} // namespace ecsact::detail
