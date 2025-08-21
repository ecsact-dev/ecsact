#include "parse-resolver-runtime/ids.hh"

#include <atomic>
#include <stdexcept>

static std::atomic_int32_t last_id = 0;

auto ecsact::interpret::details::gen_next_id_() -> int32_t {
	// Getting here would be crazy and if we were to do anything about it we'd
	// have to implement an "ID recycling" mechanism which I think is overkill.
	if(last_id == std::numeric_limits<int32_t>::max()) {
		throw std::logic_error{"Max id count reached"};
	}

	return last_id++;
}
