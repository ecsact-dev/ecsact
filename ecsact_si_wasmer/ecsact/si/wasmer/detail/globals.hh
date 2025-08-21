#pragma once

#include <wasm.h>

namespace ecsact::wasm::detail {
auto engine() -> wasm_engine_t*;
}
