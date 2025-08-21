#include "ecsact/si/wasmer/detail/globals.hh"

namespace {
wasm_engine_t* _engine = nullptr;
}

auto ecsact::wasm::detail::engine() -> wasm_engine_t* {
	if(!_engine) {
		_engine = wasm_engine_new();
	}
	return _engine;
}
