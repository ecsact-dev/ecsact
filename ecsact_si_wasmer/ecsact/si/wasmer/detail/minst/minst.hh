#pragma once

#include <string_view>
#include <string>
#include <vector>
#include <span>
#include <cstddef>
#include <functional>
#include <optional>
#include <variant>
#include <cstdint>
#include <wasm.h>

namespace ecsact::wasm::detail {

enum class minst_error_code {
	ok,
	compile_fail,
	unresolved_guest_import,
	instantiate_fail,
};

struct minst_error {
	minst_error_code code = {};
	std::string      message = {};
};

class minst_trap {
public:
	minst_trap(wasm_trap_t* trap);
	minst_trap(minst_trap&& other);
	~minst_trap();

	auto message() const -> std::string;

private:
	wasm_trap_t* trap;
};

struct minst_import_resolve_func {
	wasm_functype_t*     func_type;
	wasm_func_callback_t func_callback;

	auto as_extern(wasm_store_t* store) -> wasm_extern_t*;
};

using minst_import_resolve_t =
	std::optional<std::variant<minst_import_resolve_func>>;

struct minst_import {
	wasm_importtype_t* import_type;

	auto name() const -> std::string_view;
	auto module() const -> std::string_view;
	auto kind() const -> wasm_externkind_enum;
};

struct minst_export {
	wasm_exporttype_t* export_type;

	union {
		wasm_func_t*   func;
		wasm_global_t* global;
		wasm_table_t*  table;
		wasm_memory_t* memory;
	};

	auto name() const -> std::string_view;
	auto kind() const -> wasm_externkind_enum;

	auto func_call() -> std::optional<minst_trap>;
	auto func_call(int32_t p0) -> std::optional<minst_trap>;
};

/**
 * WebAssembly module and instance (minst)
 */
class minst {
public:
	using import_resolver_t =
		std::function<minst_import_resolve_t(const minst_import)>;

	static auto create( //
		wasm_engine_t*       engine,
		std::span<const std::byte> wasm_data,
		import_resolver_t    import_resolver
	) -> std::variant<minst, minst_error>;

	minst(minst&& other);
	~minst();

	auto imports() -> std::span<minst_import>;
	auto exports() -> std::span<minst_export>;

	/**
	 * Convenience function for invoking the exported `_initialize` function
	 * (if present.)
	 */
	auto initialize() -> std::optional<minst_trap>;

	auto find_import( //
		std::string_view module_name,
		std::string_view import_name
	) -> std::optional<minst_import>;

	auto find_export( //
		std::string_view export_name
	) -> std::optional<minst_export>;

private:
	minst();

	wasm_engine_t*   _engine = {};
	wasm_store_t*    _store = {};
	wasm_module_t*   _module = {};
	wasm_instance_t* _instance = {};

	std::vector<minst_import> _imports;
	std::vector<minst_export> _exports;
};

} // namespace ecsact::wasm::detail
