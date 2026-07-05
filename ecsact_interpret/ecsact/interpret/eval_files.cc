#include "ecsact/interpret/eval.hh"

#include <format>
#include <filesystem>
#include <vector>
#include <array>
#include <fstream>
#include <utility>
#include "ecsact/parse.h"
#include "ecsact/interpret/eval.h"
#include "ecsact/runtime/meta.h"
#include "ecsact/runtime/meta.hh"

#include "./detail/check_set.hh"
#include "./detail/fixed_stack.hh"
#include "./detail/read_util.hh"
#include "./detail/stack_util.hh"
#include "./detail/string_util.hh"
#include "./detail/eval_parse.hh"

namespace fs = std::filesystem;
using ecsact::parse_eval_error;
using namespace ecsact::detail;

static auto make_cluster_error_message(ecsact_execution_batches_error err)
	-> std::string {
	auto sys_name = std::string(
		ecsact_meta_system_name(static_cast<ecsact_system_id>(err.system_id))
	);

	if(static_cast<int32_t>(err.conflicting_system_id) == -1) {
		return std::format(
			"System '{}' cannot be part of the explicit cluster it is in",
			sys_name
		);
	}

	auto conflicting_sys_name = std::string(ecsact_meta_system_name(
		static_cast<ecsact_system_id>(err.conflicting_system_id)
	));

	auto comp_name = std::string(ecsact_meta_component_name(
		static_cast<ecsact_component_id>(err.component_id)
	));

	return std::format(
		"System '{}' conflicts with system '{}' on component '{}' and cannot be "
		"part of the same explicit cluster",
		sys_name,
		conflicting_sys_name,
		comp_name
	);
}

static void check_batches(
	int32_t                                source_index,
	eval_parse_state<std::ifstream>&       file_state,
	std::vector<ecsact::parse_eval_error>& errors
) {
	auto package_id = *file_state.package_id;
	auto err = ecsact_check_execution_batches(package_id);
	if(static_cast<int32_t>(err.system_id) != -1) {
		errors.push_back(
			ecsact::parse_eval_error{
				.eval_error = ECSACT_EVAL_ERR_INVALID_CLUSTER_SYSTEM,
				.source_index = source_index,
				.line = file_state.reader.current_line,
				.character = file_state.reader.current_character,
				.error_message = make_cluster_error_message(err),
			}
		);
	}

	for(auto sys_id : ecsact::meta::get_system_ids(package_id)) {
		auto nested_err = ecsact_check_system_execution_batches(
			static_cast<ecsact_system_like_id>(sys_id)
		);
		if(static_cast<int32_t>(nested_err.system_id) != -1) {
			errors.push_back(
				ecsact::parse_eval_error{
					.eval_error = ECSACT_EVAL_ERR_INVALID_CLUSTER_SYSTEM,
					.source_index = source_index,
					.line = file_state.reader.current_line,
					.character = file_state.reader.current_character,
					.error_message = make_cluster_error_message(nested_err),
				}
			);
		}
	}
}

auto ecsact::eval_files(std::vector<fs::path> files) -> std::vector<parse_eval_error> {
	using ecsact::detail::check_cyclic_imports;
	using ecsact::detail::check_set;
	using ecsact::detail::check_unknown_imports;
	using ecsact::detail::eval_imports;
	using ecsact::detail::eval_package_statements;
	using ecsact::detail::eval_parse_state;
	using ecsact::detail::fixed_stack;
	using ecsact::detail::get_sorted_states;
	using ecsact::detail::parse_eval_declarations;
	using ecsact::detail::parse_imports;
	using ecsact::detail::parse_package_statements;
	using ecsact::detail::stream_get_until;
	using ecsact::detail::try_top;

	std::vector<parse_eval_error>                errors;
	std::vector<eval_parse_state<std::ifstream>> file_states;
	file_states.reserve(files.size());
	for(auto file_path : files) {
		auto& file_state = file_states.emplace_back();
		file_state.file_path = file_path;
		file_state.reader.stream.open(file_path);
	}

	auto reset_readers = [&file_states] {
		for(auto& state : file_states) {
			state.reader.stream.close();
			state.reader.stream.open(state.file_path);
			state.reader.reset();
		}
	};

	parse_package_statements(file_states, errors);
	if(!errors.empty()) {
		return errors;
	}

	reset_readers();
	parse_imports(file_states, errors);
	if(!errors.empty()) {
		return errors;
	}

	reset_readers();
	check_unknown_imports(file_states, errors);
	if(!errors.empty()) {
		return errors;
	}

	check_cyclic_imports(file_states, errors);
	if(!errors.empty()) {
		return errors;
	}

	eval_package_statements(file_states, errors);
	if(!errors.empty()) {
		return errors;
	}

	reset_readers();
	auto sorted_file_states = get_sorted_states(file_states);
	auto source_index = 0;
	for(auto& file_state_ref : sorted_file_states) {
		auto& file_state = file_state_ref.get();

		eval_imports(source_index, file_state, errors);
		if(!errors.empty()) {
			return errors;
		}

		parse_eval_declarations(source_index, file_state, errors);
		if(!errors.empty()) {
			return errors;
		}

		check_batches(source_index, file_state, errors);
		if(!errors.empty()) {
			return errors;
		}

		source_index += 1;
	}

	return errors;
}
