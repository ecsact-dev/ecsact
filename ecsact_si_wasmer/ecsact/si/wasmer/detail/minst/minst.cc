#include "ecsact/si/wasmer/detail/minst/minst.hh"

#include <format>
#include <cassert>
#include <wasm.h>
#include <wasmer.h>
#include "ecsact/si/wasmer/detail/cpp_util.hh"

using ecsact::wasm::detail::minst;
using ecsact::wasm::detail::minst_export;
using ecsact::wasm::detail::minst_import;
using ecsact::wasm::detail::minst_import_resolve_func;
using ecsact::wasm::detail::minst_trap;

namespace {
auto get_wasmer_last_error_message() -> std::string {
	auto msg = std::string{};
	msg.resize(wasmer_last_error_length());
	wasmer_last_error_message(msg.data(), static_cast<int>(msg.size()));
	return msg;
}
} // namespace

auto minst_import::name() const -> std::string_view {
	auto name = wasm_importtype_name(import_type);
	return std::string_view{
		name->data,
		static_cast<size_t>(name->size),
	};
}

auto minst_import::module() const -> std::string_view {
	auto name = wasm_importtype_module(import_type);
	return std::string_view{
		name->data,
		static_cast<size_t>(name->size),
	};
}

auto minst_import::kind() const -> wasm_externkind_enum {
	return static_cast<wasm_externkind_enum>( //
		wasm_externtype_kind(wasm_importtype_type(import_type))
	);
}

auto minst_export::name() const -> std::string_view {
	auto name = wasm_exporttype_name(export_type);
	return std::string_view{
		name->data,
		static_cast<size_t>(name->size),
	};
}

auto minst_export::kind() const -> wasm_externkind_enum {
	return static_cast<wasm_externkind_enum>( //
		wasm_externtype_kind(wasm_exporttype_type(export_type))
	);
}

auto minst_export::func_call() -> std::optional<minst_trap> {
	assert(kind() == WASM_EXTERN_FUNC);

	auto args = wasm_val_vec_t{};
	auto results = wasm_val_vec_t{};
	auto trap = wasm_func_call(func, &args, &results);

	if(trap) {
		return minst_trap{trap};
	}

	return std::nullopt;
}

auto minst_export::func_call(int32_t p0) -> std::optional<minst_trap> {
	assert(kind() == WASM_EXTERN_FUNC);

	wasm_val_t     args_val[] = {WASM_I32_VAL(p0)};
	wasm_val_vec_t args = WASM_ARRAY_VEC(args_val);
	wasm_val_vec_t results = {};
	auto           trap = wasm_func_call(func, &args, &results);

	if(trap) {
		return minst_trap{trap};
	}

	return std::nullopt;
}

minst_trap::minst_trap(wasm_trap_t* trap) : trap(trap) {
}

minst_trap::minst_trap(minst_trap&& other) {
	trap = other.trap;
	other.trap = nullptr;
}

minst_trap::~minst_trap() {
	if(trap != nullptr) {
		wasm_trap_delete(trap);
		trap = nullptr;
	}
}

auto minst_import_resolve_func::as_extern( //
	wasm_store_t* store
) -> wasm_extern_t* {
	auto func = wasm_func_new(store, func_type, func_callback);
	return wasm_func_as_extern(func);
}

auto minst_trap::message() const -> std::string {
	auto trap_msg = wasm_message_t{};
	wasm_trap_message(trap, &trap_msg);
	return std::string{trap_msg.data, trap_msg.size - 1};
}

auto minst::create( //
	wasm_engine_t*       engine,
	std::span<const std::byte> wasm_data,
	import_resolver_t    import_resolver
) -> std::variant<minst, minst_error> {
	auto self = minst{};

	auto wasm_bytes = wasm_byte_vec_t{
		.size = wasm_data.size(),
		.data =
			reinterpret_cast<wasm_byte_t*>(const_cast<std::byte*>(wasm_data.data())),
	};

	self._engine = engine;
	self._store = wasm_store_new(self._engine);
	self._module = wasm_module_new(self._store, &wasm_bytes);

	if(self._module == nullptr) {
		return minst_error{
			minst_error_code::compile_fail,
			get_wasmer_last_error_message(),
		};
	}

	auto imports = wasm_importtype_vec_t{};
	auto exports = wasm_exporttype_vec_t{};
	wasm_module_imports(self._module, &imports);
	wasm_module_exports(self._module, &exports);

	self._imports.resize(imports.size);
	self._exports.resize(exports.size);

	for(size_t i = 0; exports.size > i; ++i) {
		self._exports[i].export_type = exports.data[i];
	}

	auto externs = std::vector<wasm_extern_t*>{};
	externs.reserve(imports.size);

	for(size_t i = 0; imports.size > i; ++i) {
		self._imports[i].import_type = imports.data[i];
		auto guest_import_resolve = import_resolver(self._imports[i]);

		if(!guest_import_resolve) {
			return minst_error{
				minst_error_code::unresolved_guest_import,
				std::format(
					"Guest import '{}.{}' is unresolved",
					self._imports[i].module(),
					self._imports[i].name()
				),
			};
		}

		wasm_extern_t* guest_import_extern = std::visit(
			[&](auto resolve) { return resolve.as_extern(self._store); },
			*guest_import_resolve
		);

		externs.push_back(guest_import_extern);
	}

	auto instance_externs = wasm_extern_vec_t{
		.size = externs.size(),
		.data = externs.data(),
	};

	self._instance = wasm_instance_new( //
		self._store,
		self._module,
		&instance_externs,
		nullptr
	);

	if(self._instance == nullptr) {
		return minst_error{
			minst_error_code::instantiate_fail,
			get_wasmer_last_error_message(),
		};
	}

	auto inst_exports = wasm_extern_vec_t{};
	wasm_instance_exports(self._instance, &inst_exports);

	for(size_t i = 0; exports.size > i; ++i) {
		auto& exp = self._exports[i];
		switch(exp.kind()) {
			case WASM_EXTERN_FUNC:
				exp.func = wasm_extern_as_func(inst_exports.data[i]);
				break;
			case WASM_EXTERN_GLOBAL:
				exp.global = wasm_extern_as_global(inst_exports.data[i]);
				break;
			case WASM_EXTERN_TABLE:
				exp.table = wasm_extern_as_table(inst_exports.data[i]);
				break;
			case WASM_EXTERN_MEMORY:
				exp.memory = wasm_extern_as_memory(inst_exports.data[i]);
				break;
		}
	}

	return self;
}

minst::minst() = default;

minst::minst(minst&& other) {
	_engine = other._engine;
	_store = other._store;
	_module = other._module;
	_instance = other._instance;
	_imports = std::move(other._imports);
	_exports = std::move(other._exports);

	other._engine = nullptr;
	other._store = nullptr;
	other._module = nullptr;
	other._instance = nullptr;
	other._imports = {};
	other._exports = {};
}

minst::~minst() {
	if(_instance != nullptr) {
		wasm_instance_delete(_instance);
		_instance = nullptr;
	}

	if(_module != nullptr) {
		wasm_module_delete(_module);
		_module = nullptr;
	}

	if(_store != nullptr) {
		wasm_store_delete(_store);
		_store = nullptr;
	}
}

auto minst::imports() -> std::span<minst_import> {
	return std::span{_imports.data(), _imports.size()};
}

auto minst::exports() -> std::span<minst_export> {
	return std::span{_exports.data(), _exports.size()};
}

auto minst::initialize() -> std::optional<minst_trap> {
	for(auto exp : exports()) {
		if(exp.name() == "_initialize") {
			return exp.func_call();
		}
	}

	return std::nullopt;
}

auto minst::find_import( //
	std::string_view module_name,
	std::string_view import_name
) -> std::optional<minst_import> {
	for(auto imp : imports()) {
		if(imp.module() == module_name && imp.name() == import_name) {
			return imp;
		}
	}

	return std::nullopt;
}

auto minst::find_export( //
	std::string_view export_name
) -> std::optional<minst_export> {
	for(auto exp : exports()) {
		if(exp.name() == export_name) {
			return exp;
		}
	}

	return std::nullopt;
}
