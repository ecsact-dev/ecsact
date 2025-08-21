#include "ecsact/interpret/eval.hh"

#include <filesystem>
#include <vector>
#include <array>
#include <fstream>
#include <utility>
#include "ecsact/parse.h"
#include "ecsact/interpret/eval.h"

#include "./detail/check_set.hh"
#include "./detail/fixed_stack.hh"
#include "./detail/read_util.hh"
#include "./detail/stack_util.hh"
#include "./detail/string_util.hh"
#include "./detail/eval_parse.hh"

namespace fs = std::filesystem;
using ecsact::parse_eval_error;

std::vector<parse_eval_error> ecsact::eval_files(std::vector<fs::path> files) {
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

	parse_package_statements(file_states, errors);
	if(!errors.empty()) {
		return errors;
	}

	parse_imports(file_states, errors);
	if(!errors.empty()) {
		return errors;
	}

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

		source_index += 1;
	}

	return errors;
}
