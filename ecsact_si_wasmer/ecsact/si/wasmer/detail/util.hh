#pragma once

#include <wasm.h>

namespace ecsact::wasm::detail {

inline wasm_functype_t* wasm_functype_new_4_0(
	wasm_valtype_t* p1,
	wasm_valtype_t* p2,
	wasm_valtype_t* p3,
	wasm_valtype_t* p4
) {
	wasm_valtype_t*    ps[4] = {p1, p2, p3, p4};
	wasm_valtype_vec_t params, results;
	wasm_valtype_vec_new(&params, 4, ps);
	wasm_valtype_vec_new_empty(&results);
	return wasm_functype_new(&params, &results);
}

inline wasm_functype_t* wasm_functype_new_4_1(
	wasm_valtype_t* p1,
	wasm_valtype_t* p2,
	wasm_valtype_t* p3,
	wasm_valtype_t* p4,
	wasm_valtype_t* r
) {
	wasm_valtype_t*    rs[1] = {r};
	wasm_valtype_t*    ps[4] = {p1, p2, p3, p4};
	wasm_valtype_vec_t params, results;
	wasm_valtype_vec_new(&params, 4, ps);
	wasm_valtype_vec_new(&results, 1, rs);
	return wasm_functype_new(&params, &results);
}

template<typename T>
auto wasm_memory_cast( //
	wasm_memory_t* wasm_mem,
	int32_t        wasm_guest_ptr
) -> T* {
	auto mem_bytes = wasm_memory_data(wasm_mem);
	return reinterpret_cast<T*>(mem_bytes + wasm_guest_ptr);
}
} // namespace ecsact::wasm::detail
