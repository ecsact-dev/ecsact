#pragma once

#include <cassert>
#include <string_view>
#include <vector>
#include <string>
#include <utility>
#include <concepts>
#include "ecsact/codegen/plugin.hh"
#include "ecsact/runtime/meta.hh"

namespace ecsact::cpp_codegen_plugin_util {

inline auto inc_header( //
	ecsact::codegen_plugin_context& ctx,
	auto&&                          header_path
) -> void {
	ctx.writef("#include \"{}\"\n", header_path);
}

inline auto inc_package_header( //
	ecsact::codegen_plugin_context& ctx,
	ecsact_package_id               pkg_id,
	std::string                     extension = ".hh"
) -> void {
	namespace fs = std::filesystem;

	auto main_ecsact_file_path = ecsact::meta::package_file_path(ctx.package_id);
	if(ctx.package_id == pkg_id) {
		main_ecsact_file_path.replace_extension(
			main_ecsact_file_path.extension().string() + extension
		);

		inc_header(ctx, main_ecsact_file_path.filename().string());
	} else {
		auto cpp_header_path = ecsact::meta::package_file_path(pkg_id);
		cpp_header_path.replace_extension(
			cpp_header_path.extension().string() + extension
		);
		if(main_ecsact_file_path.has_parent_path()) {
			cpp_header_path =
				fs::relative(cpp_header_path, main_ecsact_file_path.parent_path());
		}
		inc_header(ctx, cpp_header_path.filename().string());
	}
}

class method_printer {
	using parameters_list_t = std::vector<std::pair<std::string, std::string>>;

	bool                             disposed = false;
	std::optional<parameters_list_t> parameters;
	ecsact::codegen_plugin_context&  ctx;

	auto _parameter(std::string param_type, std::string param_name) -> void {
		assert(!disposed);
		if(disposed) {
			return;
		}

		assert(
			parameters.has_value() &&
			"Cannot set parameters after return type has been set"
		);
		parameters->push_back({param_type, param_name});
	}

	auto _return_type(std::string type) {
		assert(!disposed);
		if(disposed) {
			return;
		}

		assert(parameters.has_value());
		if(!parameters.has_value()) {
			return;
		}

		if(!parameters->empty()) {
			ctx.writef("\n");
		}

		for(auto i = 0; parameters->size() > i; ++i) {
			auto&& [param_type, param_name] = parameters->at(i);
			ctx.writef("\t{} {}", param_type, param_name);
			if(i + 1 < parameters->size()) {
				ctx.writef(",");
			}
			ctx.writef("\n");
		}

		parameters = std::nullopt;

		ctx.writef(") -> {} {{", type);
		ctx.indentation += 1;
		ctx.writef("\n");
	}

public:
	method_printer( //
		ecsact::codegen_plugin_context& ctx,
		std::string                     method_name
	)
		: ctx(ctx) {
		parameters.emplace();
		ctx.writef("auto {} (", method_name);
	}

	method_printer(method_printer&& other) : ctx(other.ctx) {
		disposed = other.disposed;

		if(!disposed) {
			parameters = std::move(other.parameters);
		}

		other.disposed = true;
	}

	auto parameter(
		std::string param_type,
		std::string param_name
	) & -> method_printer& {
		_parameter(param_type, param_name);
		return *this;
	}

	auto parameter(
		std::string param_type,
		std::string param_name
	) && -> method_printer {
		_parameter(param_type, param_name);
		return std::move(*this);
	}

	auto return_type(std::string type) & -> method_printer& {
		_return_type(type);
		return *this;
	}

	auto return_type(std::string type) && -> method_printer {
		_return_type(type);
		return std::move(*this);
	}

	~method_printer() {
		if(disposed) {
			return;
		}
		disposed = true;
		ctx.indentation -= 1;
		ctx.writef("\n}}\n\n");
	}
};

class block_printer {
	bool                            disposed = false;
	ecsact::codegen_plugin_context& ctx;

public:
	block_printer(ecsact::codegen_plugin_context& ctx) : ctx(ctx) {
		ctx.writef("{{");
		ctx.indentation += 1;
		ctx.writef("\n");
	}

	block_printer(block_printer&& other) : ctx(other.ctx) {
		disposed = other.disposed;
		other.disposed = true;
	}

	~block_printer() {
		if(disposed) {
			return;
		}
		disposed = true;
		ctx.indentation -= 1;
		ctx.writef("\n}}");
	}
};

auto block( //
	ecsact::codegen_plugin_context& ctx,
	std::invocable auto&&           block_body_fn
) {
	auto printer = block_printer{ctx};
	block_body_fn();
}

auto block( //
	ecsact::codegen_plugin_context& ctx,
	auto&&                          block_head,
	std::invocable auto&&           block_body_fn
) {
	ctx.writef("{} ", block_head);
	auto printer = block_printer{ctx};
	block_body_fn();
}

auto block( //
	ecsact::codegen_plugin_context& ctx,
	auto&&                          block_head,
	std::invocable auto&&           block_body_fn,
	auto&&                          block_tail
) {
	ctx.writef("{} ", block_head);
	auto printer = block_printer{ctx};
	block_body_fn();
	ctx.writef("{}", block_tail);
}

auto comma_delim(auto&& range) -> std::string {
	auto result = std::string{};

	for(auto str : range) {
		result += str + ", ";
	}

	if(result.ends_with(", ")) {
		result = result.substr(0, result.size() - 2);
	}

	return result;
}

} // namespace ecsact::cpp_codegen_plugin_util
